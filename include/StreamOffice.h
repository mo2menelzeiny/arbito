
#ifndef ARBITO_STREAMOFFICE_H
#define ARBITO_STREAMOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "PriceEvent.h"
#include "NATSSession.h"

typedef std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> PriceRingBuffer;

class StreamOffice {
public:
	explicit StreamOffice(PriceRingBuffer &priceRingBuffer);
	StreamOffice(PriceRingBuffer &priceRingBuffer,const char *host, const char *username, const char *password);

	inline void doWork() {
		m_priceEventPoller->poll(m_priceEventHandler);
	}

	void initiate();

	void terminate();

private:
	PriceRingBuffer m_priceRingBuffer;
	std::shared_ptr<Disruptor::EventPoller<PriceEvent>> m_priceEventPoller;
	std::function<bool(PriceEvent &, long, bool)> m_priceEventHandler;
	NATSSession m_natsSession;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
};

#endif //ARBITO_STREAMOFFICE_H
