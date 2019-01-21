
#include <IBMarketOffice.h>

IBMarketOffice::IBMarketOffice(
		std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> &outRingBuffer,
		const char *broker,
		double quantity
) : m_outRingBuffer(outRingBuffer),
    m_broker(broker),
    m_quantity(quantity) {
	m_consoleLogger = spdlog::get("console");
	m_systemLogger = spdlog::get("system");

	m_sequence = 0;

	m_bid = -99;
	m_bidQty = 0;
	m_offer = 99;
	m_offerQty = 0;

	m_onTickHandler = OnTickHandler([&](TickType tickType, double value) {
		switch (tickType) {
			case ASK:
				m_offer = value;
				break;
			case BID:
				m_bid = value;
				break;
			case BID_SIZE:
				m_bidQty = value;
			case ASK_SIZE:
				m_offerQty = value;
				break;
			default:
				break;
		}

		if (m_quantity > m_offerQty || m_quantity > m_bidQty) {
			return;
		}

		++m_sequence;

		auto nextSequence = m_outRingBuffer->next();
		(*m_outRingBuffer)[nextSequence].bid = m_bid;
		(*m_outRingBuffer)[nextSequence].offer = m_offer;
		(*m_outRingBuffer)[nextSequence].sequence = m_sequence;
		m_outRingBuffer->publish(nextSequence);

		m_systemLogger->info("[{}][{}] bid: {} offer: {}", m_broker, m_sequence, m_bid, m_offer);
	});

	m_onOrderStatusHandler = OnOrderStatusHandler([&](OrderId orderId, const std::string &status, double avgPrice) {
		// do nothing
	});

	m_ibClient = new IBClient(m_onTickHandler, m_onOrderStatusHandler);
}

void IBMarketOffice::cleanup() {
	m_ibClient->disconnect();
}
