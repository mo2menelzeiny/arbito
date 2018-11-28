
#include "IBTradeOffice.h"

IBTradeOffice::IBTradeOffice(
		const char *broker,
		double quantity,
		int subscriptionPort,
		const char *dbUri,
		const char *dbName
) : m_broker(broker),
    m_quantity(quantity),
    m_mongoDriver(
		    broker,
		    dbUri,
		    dbName,
		    "coll_orders"
    ) {
	sprintf(m_subscriptionURI, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPort);
}

void IBTradeOffice::start() {
	m_running = true;
	m_worker = std::thread(&IBTradeOffice::work, this);
}

void IBTradeOffice::stop() {
	m_running = false;
	m_worker.join();
}

void IBTradeOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(2, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "trade-office");

	auto systemLogger = spdlog::get("system");

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

	auto subscriptionId = aeronClient->addSubscription(m_subscriptionURI, 1);
	auto subscription = aeronClient->findSubscription(subscriptionId);
	while (!subscription) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		subscription = aeronClient->findSubscription(subscriptionId);
	}

	sbe::MessageHeader tradeDataHeader;
	sbe::TradeData tradeData;

	auto onTickHandler = OnTickHandler([&](TickType tickType, double value) {
		// do nothing
	});

	OrderId lastRecordedOrderId = 0;

	auto onOrderStatus = OnOrderStatus([&](OrderId orderId, const std::string &status, double avgFillPrice) {
		if (status != "Filled") return;
		if (lastRecordedOrderId == orderId) return;

		char orderIdStr[32];
		sprintf(orderIdStr, "%lu", orderId);

		char clOrdIdStr[32];
		sprintf(clOrdIdStr, "%lu", tradeData.id());

		std::thread([&] {
			m_mongoDriver.record(clOrdIdStr, orderIdStr, tradeData.side(), avgFillPrice);
		}).detach();

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

		systemLogger->info("{}", tradeData.side() == '1' ? "BUY" : "SELL");
	};

	auto fragmentAssembler = aeron::FragmentAssembler(fragmentHandler);

	while (m_running) {
		if (!ibClient.isConnected()) {
			bool result = ibClient.connect("127.0.0.1", 4001, 1);

			if (!result) {
				systemLogger->info("Trade Office Client FAILED");
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			systemLogger->info("Trade Office OK");
		}

		subscription->poll(fragmentAssembler.handler(), 1);
		ibClient.processMessages();
		aeronClient->conductorAgentInvoker().invoke();
	}

	ibClient.disconnect();
}
