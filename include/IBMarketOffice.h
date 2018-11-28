
#ifndef ARBITO_IBMARKETOFFICE_H
#define ARBITO_IBMARKETOFFICE_H

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/NoOpIdleStrategy.h>
#include <FragmentAssembler.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/TradeData.h"
#include "sbe/MarketData.h"

// IB
#include "IBAPI/IBClient/IBClient.h"

//SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

class IBMarketOffice {
public:
	IBMarketOffice(
			const char *broker,
			double quantity,
			int publicationPort,
			const char *publicationHost
	);

	void start();

	void stop();

private:
	void work();

private:
	const char *m_broker;
	double m_quantity;
	char m_publicationURI[64];
	std::thread m_worker;
	std::atomic_bool m_running;
};


#endif //ARBITO_IBMARKETOFFICE_H
