
#include <IBMarketOffice.h>

IBMarketOffice::IBMarketOffice(
		std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &outRingBuffer,
		const char *broker,
		double quantity
) : m_outRingBuffer(outRingBuffer),
    m_brokerStr(broker),
    m_quantity(quantity),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_sequence(0),
    m_bid(-99),
    m_bidQty(0),
    m_offer(99),
    m_offerQty(0) {
	m_onTickHandler = OnTickHandler([&](int side, double price, int size) {
		if (m_quantity > size) {
			return;
		}

		switch (side) {
			case 0:
				if (m_offer == price) {
					auto nextSequence = m_outRingBuffer->next();
					(*m_outRingBuffer)[nextSequence] = {Broker::IB, m_bid, m_offer, m_sequence};
					m_outRingBuffer->publish(nextSequence);

					m_systemLogger->info("[{}][{}]", m_brokerStr, m_sequence);
					return;
				}

				m_offer = price;
				m_offerQty = size;

				break;
			case 1:
				if (m_bid == price) {
					auto nextSequence = m_outRingBuffer->next();
					(*m_outRingBuffer)[nextSequence] = {Broker::IB, m_bid, m_offer, m_sequence};
					m_outRingBuffer->publish(nextSequence);

					m_systemLogger->info("[{}][{}]", m_brokerStr, m_sequence);
					return;
				}

				m_bid = price;
				m_bidQty = size;
				break;
			default:
				break;
		}

		++m_sequence;

		auto nextSequence = m_outRingBuffer->next();
		(*m_outRingBuffer)[nextSequence] = {Broker::IB, m_bid, m_offer, m_sequence};
		m_outRingBuffer->publish(nextSequence);

		m_systemLogger->info("[{}][{}] bid: {} offer: {}", m_brokerStr, m_sequence, m_bid, m_offer);
	});

	m_onOrderStatusHandler = OnOrderStatusHandler([&](OrderId orderId, const std::string &status, double avgPrice) {
		// do nothing
	});

	m_onErrorHandler = OnErrorHandler([&](int errorCode, const std::string &errorString) {
		switch (errorCode) {
			case 501:
			case 502:
			case 503:
			case 504:
			case 509:
			case 1100:
			case 1300:
			case 2103:
			case 2110:
				throw std::runtime_error(errorString);

			default:
				m_consoleLogger->info("[{}] Market Office {} - {}", m_brokerStr, errorCode, errorString);
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
	m_consoleLogger->info("[{}] Market Office OK", m_brokerStr);
}

void IBMarketOffice::terminate() {
	if (!m_ibClient->isConnected()) {
		return;
	}

	m_ibClient->disconnect();
}

