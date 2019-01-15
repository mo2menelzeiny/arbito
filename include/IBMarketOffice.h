
#ifndef ARBITO_IBMARKETOFFICE_H
#define ARBITO_IBMARKETOFFICE_H

// IB
#include "IBAPI/IBClient/IBClient.h"

//SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

// Disruptor
#include "Disruptor/Disruptor.h"

// Domain
#include "MarketDataEvent.h"
#include "BrokerEnum.h"

class IBMarketOffice {
public:
	IBMarketOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &ringBuffer,
			int cpuset,
			const char *broker,
			double quantity
	);

	void start();

	void stop();

private:
	void work();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_ringBuffer;
	int m_cpuset;
	const char *m_broker;
	double m_quantity;
	std::thread m_worker;
	std::atomic_bool m_running;
};


#endif //ARBITO_IBMARKETOFFICE_H
