
#ifndef ARBITO_IBOFFICE_H
#define ARBITO_IBOFFICE_H

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

#include "MongoDBDriver.h"


class IBOffice {
public:
	IBOffice(
			const char *broker,
			double quantity,
			int publicationPort,
			const char *publicationHost,
			int subscriptionPort,
			const char *DBUri,
			const char *DBName
	);

	void start();

	void stop();

private:
	void work();

private:
	const char *m_broker;
	double m_quantity;
	char m_publicationURI[64];
	char m_subscriptionURI[64];
	std::thread m_worker;
	std::atomic_bool m_running;
	MongoDBDriver m_mongoDriver;
};


#endif //ARBITO_IBOFFICE_H
