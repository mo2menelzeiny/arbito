
#ifndef ARBITO_FIXMARKETOFFICE_H
#define ARBITO_FIXMARKETOFFICE_H

// SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

// Disruptor
#include "Disruptor/Disruptor.h"

// Domain
#include "FIXSession.h"
#include "MarketDataEvent.h"

class FIXMarketOffice {
public:
	FIXMarketOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &inRingBuffer,
			int cpuset,
			const char *broker,
			double quantity,
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat
	);

	void start();

	void stop();

private:
	void work();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_inRingBuffer;
	int m_cpuset;
	const char *m_broker;
	double m_quantity;
	struct fix_field *m_MDRFields;
	struct fix_message m_MDRFixMessage{};
	FIXSession m_fixSession;
	std::thread m_worker;
	std::atomic_bool m_running;
};


#endif //ARBITO_FIXMARKETOFFICE_H
