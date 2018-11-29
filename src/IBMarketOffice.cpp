
#include "IBMarketOffice.h"

IBMarketOffice::IBMarketOffice(
		const char *broker,
		double quantity,
		int publicationPort,
		const char *publicationHost
) : m_broker(broker),
    m_quantity(quantity) {
	sprintf(m_publicationURI, "aeron:udp?endpoint=%s:%i\n", publicationHost, publicationPort);
}

void IBMarketOffice::start() {
	m_running = true;
	m_worker = std::thread(&IBMarketOffice::work, this);
}

void IBMarketOffice::stop() {
	m_running = false;
	m_worker.join();
}

void IBMarketOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "market-office");

	auto systemLogger = spdlog::get("system");
	auto marketLogger = spdlog::daily_logger_st("market", "market_log");

	long sequence = 0;

	double bid = -99, offer = 99;
	double bidQty = 0, offerQty = 0;

	aeron::Context aeronContext;

	aeronContext
			.useConductorAgentInvoker(true)
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

	aeronClient->conductorAgentInvoker().start();

	auto publicationId = aeronClient->addExclusivePublication(m_publicationURI, 1);
	auto publication = aeronClient->findExclusivePublication(publicationId);
	while (!publication) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		publication = aeronClient->findExclusivePublication(publicationId);
	}

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

		// prevents duplicate prices from IB Market Office
		if (bid == marketData.bid() && offer == marketData.offer()) return;

		++sequence;

		marketData
				.bid(bid)
				.offer(offer)
				.timestamp(sequence);

		while (publication->offer(atomicBuffer, 0, encodedLength) < -1);

		marketLogger->info("[{}][{}] bid: {} offer: {}", m_broker, sequence, bid, offer);
	});

	auto onOrderStatus = OnOrderStatus([&](OrderId orderId, const std::string &status, double avgFillPrice) {
		// do nothing
	});

	IBClient ibClient(onTickHandler, onOrderStatus);

	while (m_running) {
		if (!ibClient.isConnected()) {
			bool result = ibClient.connect("127.0.0.1", 4001, 0);

			if (!result) {
				systemLogger->info("Market Office Client FAILED");
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			ibClient.subscribeToFeed();
			systemLogger->info("Market Office OK");
		}

		ibClient.processMessages();
		aeronClient->conductorAgentInvoker().invoke();
	}

	ibClient.disconnect();
}
