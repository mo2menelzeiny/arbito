
#ifndef ARBITO_IBMARKETOFFICE_H
#define ARBITO_IBMARKETOFFICE_H

//SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Disruptor
#include <Disruptor/Disruptor.h>

// IB
#include "IBAPI/IBClient/IBClient.h"

// Domain
#include "MarketEvent.h"

class IBMarketOffice {
public:
	IBMarketOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> &outRingBuffer,
			const char *broker,
			double quantity
	);

	inline void doWork() {
		if (!m_ibClient->isConnected()) {
			throw std::runtime_error("API Client disconnected FAILED");
		}

		m_ibClient->processMessages();
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketEvent>> m_outRingBuffer;
	const char *m_broker;
	double m_quantity;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	long m_sequence;
	double m_bid;
	double m_bidQty;
	double m_offer;
	double m_offerQty;
	IBClient *m_ibClient;
	OnTickHandler m_onTickHandler;
	OnOrderStatusHandler m_onOrderStatusHandler;
};


#endif //ARBITO_IBMARKETOFFICE_H
