
#include <BusinessOffice.h>

BusinessOffice::BusinessOffice(
		std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &priceRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> &executionRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &orderRingBuffer,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose,
		const char *dbUri,
		const char *dbName
) : m_priceRingBuffer(priceRingBuffer),
    m_executionRingBuffer(executionRingBuffer),
    m_orderRingBuffer(orderRingBuffer),
    m_priceEventPoller(m_priceRingBuffer->newPoller()),
    m_executionEventPoller(m_executionRingBuffer->newPoller()),
    m_orderDelaySec(orderDelaySec),
    m_windowMs(stoi(getenv("WINDOW_MS"))),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose),
    m_mongoDriver(dbUri, dbName, "triggers"),
    m_idGenerator(std::random_device{}()),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_currentOrder(Difference::NONE),
    m_isOrderDelayed(false),
    m_lastOrderTime(time(nullptr)),
    m_priceA{Broker::NONE, -99, 99, 0},
    m_priceB{Broker::NONE, -99, 99, 0},
    m_priceBTrunc{Broker::NONE, -99, 99, 0},
    m_ordersCount(std::stoi(getenv("ORDERS_COUNT"))),
    m_currentDiff(getDifference(getenv("CURRENT_DIFF"))),
    m_isExpiredB(true),
    m_canLogPrices(false),
    m_sequence(0) {

	m_priceEventHandler = [&](PriceEvent &event, int64_t seq, bool endOfBatch) -> bool {
		switch (event.broker) {
			case Broker::LMAX:
				m_priceA = event;
				break;

			case Broker::IB:
				m_priceB = event;
				/*sprintf(m_truncStrBuff, "%lf", event.bid);
				m_truncStrBuff[6] = '0';
				m_priceBTrunc.bid = std::stof(m_truncStrBuff);

				sprintf(m_truncStrBuff, "%lf", event.offer);
				m_truncStrBuff[6] = '0';
				m_priceBTrunc.offer = std::stof(m_truncStrBuff);*/

				/*m_isExpiredB = false;
				m_timestampB = m_timestampNow;*/
				break;

			default:
				break;
		}

		m_canLogPrices = true;

		/*m_systemLogger->info(
				"DiffA: {} DiffB: {} DiffAT: {} DiffBT: {} SeqA: {} SeqB: {}",
				(m_priceA.bid - m_priceB.offer) * 100000,
				(m_priceB.bid - m_priceA.offer) * 100000,
				(m_priceA.bid - m_priceBTrunc.offer) * 100000,
				(m_priceBTrunc.bid - m_priceA.offer) * 100000,
				m_priceA.sequence,
				m_priceB.sequence
		);*/

		return true;
	};

	m_executionEventHandler = [&](ExecutionEvent &event, int64_t seq, bool endOfBatch) -> bool {
		if (event.isFilled && event.id != CORRECTION_ID) {
			++m_ordersCount;
			return true;
		}

		if (event.isFilled && event.id == CORRECTION_ID) {
			--m_ordersCount;
			return true;
		}

		if (event.id == CORRECTION_ID) {
			m_consoleLogger->error("Business Office Correction FAILED");
			return true;
		}

		auto nextSequence = m_orderRingBuffer->next();
		(*m_orderRingBuffer)[nextSequence] = {
				event.broker == Broker::IB ? Broker::LMAX : Broker::IB,
				OrderType::MARKET,
				event.side,
				0,
				CORRECTION_ID
		};
		m_orderRingBuffer->publish(nextSequence);

		return true;
	};
}

void BusinessOffice::initiate() {
	m_priceRingBuffer->addGatingSequences({m_priceEventPoller->sequence()});
	m_executionRingBuffer->addGatingSequences({m_executionEventPoller->sequence()});
	m_consoleLogger->info("Business Office OK");
}

void BusinessOffice::terminate() {
	m_priceRingBuffer->removeGatingSequence({m_priceEventPoller->sequence()});
	m_executionRingBuffer->removeGatingSequence({m_executionEventPoller->sequence()});
}

