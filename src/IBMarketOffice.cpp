
#include <IBMarketOffice.h>

IBMarketOffice::IBMarketOffice(
		std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> &outRingBuffer,
		const char *broker,
		double quantity
) : m_outRingBuffer(outRingBuffer),
    m_broker(broker),
    m_quantity(quantity),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_sequence(0),
    m_bid(-99),
    m_bidQty(0),
    m_offer(99),
    m_offerQty(0) {
	m_onTickHandler = OnTickHandler([&](int side, double price, int size) {
		switch (side) {
			case 0:
				m_offer = price;
				m_offerQty = size;
				break;
			case 1:
				m_bid = price;
				m_bidQty = size;
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
		(*m_outRingBuffer)[nextSequence].broker = IB;
		m_outRingBuffer->publish(nextSequence);

		m_systemLogger->info("[{}][{}] bid: {} offer: {}", m_broker, m_sequence, m_bid, m_offer);
	});

	m_onOrderStatusHandler = OnOrderStatusHandler([&](OrderId orderId, const std::string &status, double avgPrice) {
		// do nothing
	});

	m_onErrorHandler = OnErrorHandler([&](int errorCode, const std::string &errorString) {
		switch (errorCode) {
			case 2104:
			case 2106:
				break;
			default:
				m_consoleLogger->error("[{}] Market Office FAILED {} {}", m_broker, errorCode, errorString);
				break;
		}
	});

	m_ibClient = new IBClient(m_onTickHandler, m_onOrderStatusHandler, m_onErrorHandler);
}

void IBMarketOffice::initiate() {
	if (!m_ibClient->connect("127.0.0.1", 4002, 0)) {
		throw std::runtime_error("API Client connect FAILED");
	}

	m_ibClient->subscribeToFeed();
	m_consoleLogger->info("[{}] Market Office OK", m_broker);
}

void IBMarketOffice::terminate() {
	if (!m_ibClient->isConnected()) {
		return;
	}

	m_ibClient->disconnect();
}

