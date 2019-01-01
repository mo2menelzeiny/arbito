
#include "IBMarketOffice.h"

IBMarketOffice::IBMarketOffice(
		std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &ringBuffer,
		int cpuset,
		const char *broker,
		double quantity
) : m_ringBuffer(ringBuffer),
    m_broker(broker),
    m_quantity(quantity) {
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
	CPU_SET(m_cpuset, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "market-office");

	auto consoleLogger = spdlog::get("console");
	auto systemLogger = spdlog::get("system");

	long sequence = 0;

	double bid = -99, offer = 99;
	double bidQty = 0, offerQty = 0;

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

		++sequence;

		auto nextSequence = m_ringBuffer->next();
		(*m_ringBuffer)[nextSequence].bid = bid;
		(*m_ringBuffer)[nextSequence].offer = offer;
		(*m_ringBuffer)[nextSequence].sequence = sequence;
		m_ringBuffer->publish(nextSequence);

		systemLogger->info("[{}][{}] bid: {} offer: {}", m_broker, sequence, bid, offer);
	});

	auto onOrderStatus = OnOrderStatus([&](OrderId orderId, const std::string &status, double avgFillPrice) {
		// do nothing
	});

	IBClient ibClient(onTickHandler, onOrderStatus);

	while (m_running) {
		if (!ibClient.isConnected()) {
			bool result = ibClient.connect("127.0.0.1", 4002, 0);

			if (!result) {
				consoleLogger->info("[{}] Market Office Client FAILED", m_broker);
				std::this_thread::sleep_for(std::chrono::seconds(30));
				continue;
			}

			ibClient.subscribeToFeed();
			consoleLogger->info("[{}] Market Office OK", m_broker);
		}

		ibClient.processMessages();
	}

	ibClient.disconnect();
}
