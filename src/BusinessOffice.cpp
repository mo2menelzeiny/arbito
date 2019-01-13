
#include <BusinessOffice.h>

BusinessOffice::BusinessOffice(
		std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &inRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &outRingBuffer,
		int windowMs,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose,
		const char *dbUri,
		const char *dbName
) : m_inRingBuffer(inRingBuffer),
    m_outRingBuffer(outRingBuffer),
    m_windowMs(windowMs),
    m_orderDelaySec(orderDelaySec),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose),
    m_mongoDriver(
		    dbUri,
		    dbName,
		    "coll_triggers"
    ),
    m_idGenerator(std::random_device{}()) {

	m_consoleLogger = spdlog::get("console");
	m_systemLogger = spdlog::get("system");

	m_ordersCount = stoi(getenv("ORDERS_COUNT"));

	m_currentDiff = DIFF_NONE;
	m_currentOrder = DIFF_NONE;

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_A")) {
		m_currentDiff = DIFF_A;
	}

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_B")) {
		m_currentDiff = DIFF_B;
	}

	m_isOrderDelayed = false;
	m_lastOrderTime = time(nullptr);

	m_marketDataEventPoller = m_inRingBuffer->newPoller();
	m_inRingBuffer->addGatingSequences({m_marketDataEventPoller->sequence()});

	m_marketDataEventHandler = [&](MarketDataEvent &marketDataEvent, int64_t seq, bool endOfBatch) -> bool {
		switch (marketDataEvent.broker) {
			case LMAX:
				m_marketDataA = marketDataEvent;
				break;
			case IB:
				m_marketDataB = marketDataEvent;
				break;
			case SWISSQUOTE:
			case NONE:
				break;
		}

		return false;
	};

	m_marketDataA.bid = -99;
	m_marketDataA.offer = 99;

	m_marketDataB.bid = -99;
	m_marketDataB.offer = 99;

	m_consoleLogger->info("Business Office OK");
}

void BusinessOffice::cleanup() {
}
