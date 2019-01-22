
#include "IBTradeOffice.h"

IBTradeOffice::IBTradeOffice(
		std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &inRingBuffer,
		int cpuset,
		const char *broker,
		double quantity,
		const char *dbUri,
		const char *dbName
) : m_inRingBuffer(inRingBuffer),
    m_cpuset(cpuset),
    m_broker(broker),
    m_quantity(quantity),
    m_mongoDriver(
		    dbUri,
		    dbName,
		    "coll_orders"
    ) {
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
	CPU_SET(m_cpuset, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "trade-office");

	auto consoleLogger = spdlog::get("console");
	auto systemLogger = spdlog::get("system");

	auto onTickHandler = OnTickHandler([&](int side, double price, int size) {
		// do nothing
	});

	auto onErrorHandler = OnErrorHandler([&](int errorCode, const std::string &errorString) {
		// do nothing
	});

	char orderIdStr[64];
	char clOrdIdStr[64];

	OrderId lastRecordedOrderId = 0;
	char lastSide;

	auto onOrderStatus = OnOrderStatusHandler([&](OrderId orderId, const std::string &status, double avgFillPrice) {
		if (status != "Filled") return;
		if (lastRecordedOrderId == orderId) return;

		sprintf(orderIdStr, "%lu", orderId);

		std::thread([=, mongoDriver = &m_mongoDriver] {
			mongoDriver->record(clOrdIdStr, orderIdStr, lastSide, avgFillPrice, m_broker);
		}).detach();

		lastRecordedOrderId = orderId;
	});

	IBClient ibClient(onTickHandler, onOrderStatus, onErrorHandler);

	auto businessPoller = m_inRingBuffer->newPoller();

	m_inRingBuffer->addGatingSequences({businessPoller->sequence()});

	auto businessHandler = [&](OrderEvent &event, int64_t seq, bool endOfBatch) -> bool {
		if (event.buy == IB) {
			ibClient.placeMarketOrder("BUY", m_quantity);
		}

		if (event.sell == IB) {
			ibClient.placeMarketOrder("SELL", m_quantity);
		}

		const char *side = event.buy == IB ? "BUY" : "SELL";
		lastSide = event.buy == IB ? '1' : '2';
		sprintf(clOrdIdStr, "%lu", event.id);

		systemLogger->info("[{}] {} id: {}", m_broker, side, clOrdIdStr);
		consoleLogger->info("[{}] {}", m_broker, side);

		return false;
	};

	while (m_running) {
		if (!ibClient.isConnected()) {
			bool result = ibClient.connect("127.0.0.1", 4002, 1);

			if (!result) {
				consoleLogger->info("[{}] Trade Office Client FAILED", m_broker);
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			consoleLogger->info("[{}] Trade Office OK", m_broker);
		}

		ibClient.processMessages();
		businessPoller->poll(businessHandler);
	}

	ibClient.disconnect();
	m_inRingBuffer->removeGatingSequence(businessPoller->sequence());
}
