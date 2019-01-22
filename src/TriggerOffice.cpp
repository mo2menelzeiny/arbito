
#include <TriggerOffice.h>

TriggerOffice::TriggerOffice(
		std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &inRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &outRingBuffer,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose,
		const char *dbUri,
		const char *dbName
) : m_inRingBuffer(inRingBuffer),
    m_outRingBuffer(outRingBuffer),
    m_orderDelaySec(orderDelaySec),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose),
    m_mongoDriver(dbUri, dbName, "coll_triggers"),
    m_idGenerator(std::random_device{}()),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_currentDiff(DIFF_NONE),
    m_currentOrder(DIFF_NONE),
    m_isOrderDelayed(false),
    m_lastOrderTime(time(nullptr)),
    m_marketEventPoller(m_inRingBuffer->newPoller()),
    m_marketDataA{.bid = -99, .offer = 99},
    m_marketDataB{.bid = -99, .offer = 99},
    m_marketDataBTrunc{.bid = -99, .offer = 99} {

	// TODO: temporary to keep state
	m_ordersCount = stoi(getenv("ORDERS_COUNT"));

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_A")) {
		m_currentDiff = DIFF_A;
	}

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_B")) {
		m_currentDiff = DIFF_B;
	}

	m_marketEventHandler = [&](PriceEvent &event, int64_t seq, bool endOfBatch) -> bool {
		switch (event.broker) {
			case LMAX:
				m_marketDataA = event;
				break;

			case IB:
				sprintf(m_truncStrBuff, "%lf", event.bid);
				m_truncStrBuff[6] = '0';
				m_marketDataBTrunc.bid = stof(m_truncStrBuff);

				sprintf(m_truncStrBuff, "%lf", event.offer);
				m_truncStrBuff[6] = '0';
				m_marketDataBTrunc.offer = stof(m_truncStrBuff);;

				m_marketDataB = event;
				break;

			default:
				break;
		}

		return false;
	};
}

void TriggerOffice::initiate() {
	m_inRingBuffer->addGatingSequences({m_marketEventPoller->sequence()});
	m_consoleLogger->info("Business Office OK");
}

void TriggerOffice::terminate() {
	m_inRingBuffer->removeGatingSequence({m_marketEventPoller->sequence()});
}

