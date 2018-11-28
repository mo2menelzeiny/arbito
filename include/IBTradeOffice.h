
#ifndef ARBITO_IBTRADEOFFICE_H
#define ARBITO_IBTRADEOFFICE_H

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

// Mongo
#include "MongoDBDriver.h"


class IBTradeOffice {
public:
	IBTradeOffice(
			const char *broker,
			double quantity,
			int subscriptionPort,
			const char *dbUri,
			const char *dbName
	);

	void start();

	void stop();

private:
	void work();

private:
	const char *m_broker;
	double m_quantity;
	char m_subscriptionURI[64];
	std::thread m_worker;
	std::atomic_bool m_running;
	MongoDBDriver m_mongoDriver;
};


#endif //ARBITO_IBTRADEOFFICE_H
