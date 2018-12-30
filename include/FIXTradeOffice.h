
#ifndef ARBITO_FIXTRADEOFFICE_H
#define ARBITO_FIXTRADEOFFICE_H

// Disruptor
#include "Disruptor/Disruptor.h"

// Domain
#include "FIXSession.h"
#include "MongoDBDriver.h"
#include "BusinessEvent.h"

// SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

class FIXTradeOffice {
public:
	FIXTradeOffice(
			std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &inRingBuffer,
			int cpuset,
			const char *broker,
			double quantity,
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat,
			const char *dbUri,
			const char *dbName
	);

	void start();

	void stop();

private:
	void work();

private:
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_inRingBuffer;
	int m_cpuset;
	const char *m_broker;
	double m_quantity;
	struct fix_field *m_NOSSFields;
	struct fix_message m_NOSSFixMessage{};
	struct fix_field *m_NOSBFields;
	struct fix_message m_NOSBFixMessage{};
	FIXSession m_fixSession;
	MongoDBDriver m_mongoDriver;
	std::thread m_worker;
	std::atomic_bool m_running;
};


#endif //ARBITO_FIXTRADEOFFICE_H
