
#include "BusinessOffice.h"

BusinessOffice::BusinessOffice(
		std::shared_ptr <Disruptor::RingBuffer<MarketDataEvent>> &inRingBuffer,
		std::shared_ptr <Disruptor::RingBuffer<BusinessEvent>> &outRingBuffer,
		int cpuset,
		int windowMs,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose,
		const char *dbUri,
		const char *dbName
) : m_inRingBuffer(inRingBuffer),
	m_outRingBuffer(outRingBuffer),
    m_cpuset(cpuset),
    m_windowMs(windowMs),
    m_orderDelaySec(orderDelaySec),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose),
    m_mongoDriver(
		    dbUri,
		    dbName,
		    "coll_triggers"
    ) {
}

void BusinessOffice::start() {
	m_running = true;
	m_worker = std::thread(&BusinessOffice::work, this);
}

void BusinessOffice::stop() {
	m_running = false;
	m_worker.join();
}

void BusinessOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(m_cpuset, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "business-office");

	MarketDataEvent marketDataA = (MarketDataEvent) {
			.bid = -99,
			.offer = 99,
			.sequence = 0,
			.broker = NONE
	};

	MarketDataEvent marketDataB = (MarketDataEvent) {
			.bid = -99,
			.offer = 99,
			.sequence = 0,
			.broker = NONE
	};

//	bool isExpiredB = true;
//	time_point<system_clock> timestampB, timestampNow;
//	timestampB = timestampNow = system_clock::now();

	int ordersCount = stoi(getenv("ORDERS_COUNT"));

	TriggerDifference currentDiff = DIFF_NONE;

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_A")) currentDiff = DIFF_A;

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_B")) currentDiff = DIFF_B;

	TriggerDifference currentOrder = DIFF_NONE;

	bool isOrderDelayed = false;
	time_t orderDelay = m_orderDelaySec;
	time_t orderDelayStart = time(nullptr);

	auto consoleLogger = spdlog::get("console");
	auto systemLogger = spdlog::get("system");

	std::mt19937_64 randomGenerator(std::random_device{}());

	char randomIdStr[64];

	auto handleTriggers = [&] {
		if (isOrderDelayed && ((time(nullptr) - orderDelayStart) < orderDelay)) return;

		isOrderDelayed = false;

		bool canOpen = ordersCount < m_maxOrders;

		double diffA = marketDataA.bid - (std::trunc(10000 * static_cast<float>(marketDataB.offer)) / 10000);
		double diffB = (std::trunc(10000 * static_cast<float>(marketDataB.bid)) / 10000) - marketDataA.offer;

		switch (currentDiff) {
			case DIFF_A:
				if (canOpen && diffA >= m_diffOpen) {
					currentOrder = DIFF_A;
					++ordersCount;
					break;
				}

				if (diffB >= m_diffClose) {
					currentOrder = DIFF_B;
					--ordersCount;
					break;
				}

			case DIFF_B:
				if (canOpen && diffB >= m_diffOpen) {
					currentOrder = DIFF_B;
					++ordersCount;
					break;
				}

				if (diffA >= m_diffClose) {
					currentOrder = DIFF_A;
					--ordersCount;
					break;
				}

			case DIFF_NONE:
				if (diffA >= m_diffOpen) {
					currentDiff = DIFF_A;
					currentOrder = DIFF_A;
					++ordersCount;
					break;
				}

				if (diffB >= m_diffOpen) {
					currentDiff = DIFF_B;
					currentOrder = DIFF_B;
					++ordersCount;
					break;
				}

		}

		if (ordersCount == 0) {
			currentDiff = DIFF_NONE;
		}

		if (currentOrder == DIFF_NONE) return;

		const char *orderType = currentOrder == currentDiff ? "OPEN" : "CLOSE";

		auto randomId = randomGenerator();

		sprintf(randomIdStr, "%lu", randomId);

		auto nextSequence = m_outRingBuffer->next();

		(*m_outRingBuffer)[nextSequence].id = randomId;

		switch (currentOrder) {
			case DIFF_A:
				(*m_outRingBuffer)[nextSequence].sell = marketDataA.broker;
				(*m_outRingBuffer)[nextSequence].buy = marketDataB.broker;
				m_outRingBuffer->publish(nextSequence);

				systemLogger->info(
						"{} bid A: {} sequence: {} offer B: {} sequence: {}",
						orderType,
						marketDataA.bid,
						marketDataA.sequence,
						marketDataB.offer,
						marketDataB.sequence
				);

				std::thread([=, mongoDriver = &m_mongoDriver] {
					mongoDriver->record(randomIdStr, marketDataA.bid, marketDataB.offer, orderType);
				}).detach();

				break;

			case DIFF_B:
				(*m_outRingBuffer)[nextSequence].sell = marketDataB.broker;
				(*m_outRingBuffer)[nextSequence].buy = marketDataA.broker;
				m_outRingBuffer->publish(nextSequence);

				systemLogger->info(
						"{} bid B: {} sequence: {} offer A: {} sequence: {}",
						orderType,
						marketDataB.bid,
						marketDataB.sequence,
						marketDataA.offer,
						marketDataA.sequence
				);

				std::thread([=, mongoDriver = &m_mongoDriver] {
					mongoDriver->record(randomIdStr, marketDataB.bid, marketDataA.offer, orderType);
				}).detach();

				break;

			case DIFF_NONE:
				break;
		}

		marketDataA.bid = -99;
		marketDataA.offer = 99;

		marketDataB.bid = -99;
		marketDataB.offer = 99;

		currentOrder = DIFF_NONE;
		orderDelayStart = time(nullptr);
		isOrderDelayed = true;

		consoleLogger->info("{}", orderType);
	};

	auto marketDataPoller = m_inRingBuffer->newPoller();

	m_inRingBuffer->addGatingSequences({marketDataPoller->sequence()});

	auto marketDataHandler = [&](MarketDataEvent &event, int64_t seq, bool endOfBatch) -> bool {
		switch (event.broker) {
			case LMAX:
				marketDataA = event;
				break;
			case IB:
				marketDataB = event;
//				timestampB = timestampNow;
//				isExpiredB = false;
				break;
			case SWISSQUOTE:
			case NONE:
				break;
		}

		return false;
	};

	consoleLogger->info("Business Office OK");

	while (m_running) {
//		timestampNow = system_clock::now();
//
//		if (!isExpiredB && duration_cast<milliseconds>(timestampNow - timestampB).count() >= m_windowMs) {
//			marketDataB.bid = -99;
//			marketDataB.offer = 99;
//			isExpiredB = true;
//		}

		marketDataPoller->poll(marketDataHandler);
		handleTriggers();
	}

	m_inRingBuffer->removeGatingSequence(marketDataPoller->sequence());
}
