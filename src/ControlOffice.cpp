
#include <ControlOffice.h>

ControlOffice::ControlOffice(
		std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> &inRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &outRingBuffer
) : m_inRingBuffer(inRingBuffer),
    m_outRingBuffer(outRingBuffer),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_executionEventPoller(m_inRingBuffer->newPoller()) {

	m_executionEventHandler = [&](ExecutionEvent &event, int64_t seq, bool endOfBatch) -> bool {
		if (event.isFilled && event.id == 99999) {
			return false;
		}

		auto nextSequence = m_outRingBuffer->next();
		(*m_outRingBuffer)[nextSequence] = {
				event.broker == Broker::IB ? Broker::LMAX : Broker::IB,
				OrderType::MARKET,
				event.side == OrderSide::BUY ? OrderSide::SELL : OrderSide::BUY,
				0,
				99999
		};
		m_outRingBuffer->publish(nextSequence);

		m_systemLogger->info("Corrected Failed Order id {}", event.id);

		return false;
	};
}

void ControlOffice::initiate() {
	m_inRingBuffer->addGatingSequences({m_executionEventPoller->sequence()});
	m_consoleLogger->info("Control Office OK");
}

void ControlOffice::terminate() {
	m_inRingBuffer->removeGatingSequence({m_executionEventPoller->sequence()});
}

