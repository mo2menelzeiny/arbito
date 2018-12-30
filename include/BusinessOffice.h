
#ifndef ARBITO_BUSINESSOFFICE_H
#define ARBITO_BUSINESSOFFICE_H

// SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

// Disruptor
#include "Disruptor/Disruptor.h"

// Domain
#include "BusinessEvent.h"
#include "MarketDataEvent.h"
#include "TriggerDifference.h"

// MongoC
#include "MongoDBDriver.h"

using namespace std;
using namespace std::chrono;

class BusinessOffice {
public:
	BusinessOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &inRingBuffer,
			std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &outRingBuffer,
			int cpuset,
			int windowMs,
			int orderDelaySec,
			int maxOrders,
			double diffOpen,
			double diffClose,
			const char *dbUri,
			const char *dbName
	);

	void start();

	void stop();

private:
	void work();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_inRingBuffer;
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_outRingBuffer;
	int m_cpuset;
	int m_windowMs;
	int m_orderDelaySec;
	int m_maxOrders;
	double m_diffOpen;
	double m_diffClose;
	std::thread m_worker;
	std::atomic_bool m_running;
	MongoDBDriver m_mongoDriver;
};


#endif //ARBITO_BUSINESSOFFICE_H
