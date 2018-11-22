
#include <IBOffice.h>

#include "IBOffice.h"

IBOffice::IBOffice(
		const char *broker,
		double quantity,
		int publicationPort,
		const char *publicationHost,
		int subscriptionPort,
		const char *DBUri,
		const char *DBName
) : m_broker(broker),
    m_quantity(quantity),
    m_mongoDriver(
		    broker,
		    DBUri,
		    DBName,
		    "coll_orders"
    ) {
	sprintf(m_publicationURI, "aeron:udp?endpoint=%s:%i\n", publicationHost, publicationPort);
	sprintf(m_subscriptionURI, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPort);
}

void IBOffice::start() {
	m_running = true;
	m_worker = std::thread(&IBOffice::work, this);
}


void IBOffice::stop() {
	m_running = false;
	m_worker.join();
}

void IBOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "ib-office");

	auto systemLogger = spdlog::get("system");

	double bid = 0, offer = 0;
	double bidQty = 0, offerQty = 0;

	aeron::Context aeronContext;

	aeronContext
			.availableImageHandler([&](aeron::Image &image) {
				systemLogger->info("{} available", image.sourceIdentity());
			})
			.unavailableImageHandler([&](aeron::Image &image) {
				systemLogger->error("{} unavailable", image.sourceIdentity());
			})
			.errorHandler([&](const std::exception &ex) {
				systemLogger->error("{}", ex.what());
			});

	auto aeronClient = std::make_shared<aeron::Aeron>(aeronContext);

	auto publicationId = aeronClient->addExclusivePublication(m_publicationURI, 1);
	auto publication = aeronClient->findExclusivePublication(publicationId);
	while (!publication) {
		std::this_thread::yield();
		publication = aeronClient->findExclusivePublication(publicationId);
	}

	auto subscriptionId = aeronClient->addSubscription(m_subscriptionURI, 1);
	auto subscription = aeronClient->findSubscription(subscriptionId);
	while (!subscription) {
		std::this_thread::yield();
		subscription = aeronClient->findSubscription(subscriptionId);
	}

	sbe::MessageHeader tradeDataHeader;
	sbe::TradeData tradeData;

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);

	auto marketDataMessageBuffer = reinterpret_cast<char *>(buffer);

	sbe::MessageHeader marketDataHeader;
	sbe::MarketData marketData;

	marketDataHeader
			.wrap(marketDataMessageBuffer, 0, 0, 64)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	marketData.wrapForEncode(marketDataMessageBuffer, sbe::MessageHeader::encodedLength(), 64);

	auto encodedLength = static_cast<aeron::index_t>(sbe::MessageHeader::encodedLength() + marketData.encodedLength());

	auto onTickHandler = OnTickHandler([&](TickType tickType, double value) {
		switch (tickType) {
			case ASK:
				offer = value;
				break;
			case BID:
				bid = value;
				break;
			case BID_SIZE:
				bidQty = value;
			case ASK_SIZE:
				offerQty = value;
				break;
			default:
				break;
		}

		if (m_quantity > offerQty || m_quantity > bidQty) return;

		marketData
				.bid(bid)
				.offer(offer);

		while (publication->offer(atomicBuffer, 0, encodedLength) < -1);
	});

	OrderId lastRecordedOrderId = 0;

	auto onOrderStatus = OnOrderStatus([&](OrderId orderId, const std::string &status, double avgFillPrice) {
		if (status != "Filled") return;
		if (lastRecordedOrderId == orderId) return;

		char orderIdStr[32];
		sprintf(orderIdStr, "%li", orderId);

		char clOrdIdStr[32];
		sprintf(clOrdIdStr, "%li", tradeData.id());

		m_mongoDriver.recordExecution(clOrdIdStr, orderIdStr, tradeData.side(), avgFillPrice);

		lastRecordedOrderId = orderId;
	});

	IBClient ibClient(onTickHandler, onOrderStatus);

	auto fragmentHandler = [&](
			aeron::AtomicBuffer &buffer,
			aeron::index_t offset,
			aeron::index_t length,
			const aeron::Header &header
	) {
		auto bufferLength = static_cast<const uint64_t>(length);
		auto messageBuffer = reinterpret_cast<char *>(buffer.buffer() + offset);

		tradeDataHeader.wrap(messageBuffer, 0, 0, bufferLength);
		tradeData.wrapForDecode(
				messageBuffer,
				sbe::MessageHeader::encodedLength(),
				tradeDataHeader.blockLength(),
				tradeDataHeader.version(),
				bufferLength
		);

		switch (tradeData.side()) {
			case '1':
				ibClient.placeMarketOrder("BUY", m_quantity);
				break;
			case '2':
				ibClient.placeMarketOrder("SELL", m_quantity);
				break;
			default:
				break;
		}

	};

	auto fragmentAssembler = aeron::FragmentAssembler(fragmentHandler);

	while (m_running) {
		if (!ibClient.isConnected()) {
			bool result = ibClient.connect("127.0.0.1", 4001);

			if (!result) {
				systemLogger->info("IB Office: Client FAILED");
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			ibClient.subscribeToFeed();
			systemLogger->info("IB Office OK");
		}

		subscription->poll(fragmentAssembler.handler(), 1);
		ibClient.processMessages();
	}

	ibClient.disconnect();
}
