
#ifndef ARBITO_CONTROLOFFICE_H
#define ARBITO_CONTROLOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "MongoDBDriver.h"
#include "OrderEvent.h"
#include "ExecutionEvent.h"

class ControlOffice {
public:
	ControlOffice(
			std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> &inRingBuffer,
			std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &outRingBuffer
	);

	inline void doWork() {
		m_executionEventPoller->poll(m_executionEventHandler);
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> m_inRingBuffer;
	std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> m_outRingBuffer;
	std::shared_ptr<Disruptor::EventPoller<ExecutionEvent>> m_executionEventPoller;
	std::function<bool(ExecutionEvent &, long, bool)> m_executionEventHandler;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
};


#endif //ARBITO_CONTROLOFFICE_H
