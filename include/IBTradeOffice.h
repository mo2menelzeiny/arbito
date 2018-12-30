
#ifndef ARBITO_IBTRADEOFFICE_H
#define ARBITO_IBTRADEOFFICE_H

// Disruptor
#include "Disruptor/Disruptor.h"

// IB
#include "IBAPI/IBClient/IBClient.h"

//SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

// Mongo
#include "MongoDBDriver.h"
#include "BusinessEvent.h"


class IBTradeOffice {
public:
	IBTradeOffice(
			std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &inRingBuffer,
			int cpuset,
			const char *broker,
			double quantity,
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
	std::thread m_worker;
	std::atomic_bool m_running;
	MongoDBDriver m_mongoDriver;
};


#endif //ARBITO_IBTRADEOFFICE_H
