
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
    m_currentDiff(Difference::NONE),
    m_currentOrder(Difference::NONE),
    m_isOrderDelayed(false),
    m_lastOrderTime(time(nullptr)),
    m_priceEventPoller(m_inRingBuffer->newPoller()),
    m_priceA{Broker::NONE, -99, 99, 0},
    m_priceB{Broker::NONE, -99, 99, 0},
    m_priceBTrunc{Broker::NONE, -99, 99, 0} {

	// TODO: temporary to keep state
	m_ordersCount = stoi(getenv("ORDERS_COUNT"));

	m_currentDiff = getDifference(getenv("CURRENT_DIFF"));

	m_priceEventHandler = [&](PriceEvent &event, int64_t seq, bool endOfBatch) -> bool {
		switch (event.broker) {
			case Broker::LMAX:
				m_priceA = event;
				break;

			case Broker::IB:
				sprintf(m_truncStrBuff, "%lf", event.bid);
				m_truncStrBuff[6] = '0';
				m_priceBTrunc.bid = stof(m_truncStrBuff);

				sprintf(m_truncStrBuff, "%lf", event.offer);
				m_truncStrBuff[6] = '0';
				m_priceBTrunc.offer = stof(m_truncStrBuff);;

				m_priceB = event;
				break;

			default:
				break;
		}

		return false;
	};
}

void TriggerOffice::initiate() {
	m_inRingBuffer->addGatingSequences({m_priceEventPoller->sequence()});
	m_consoleLogger->info("Trigger Office OK");
}

void TriggerOffice::terminate() {
	m_inRingBuffer->removeGatingSequence({m_priceEventPoller->sequence()});
}

