
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


class IBOffice {
public:
	IBOffice(
			const char *broker,
			double spread,
			double quantity,
			int publicationPort,
			const char *publicationHost,
			int subscriptionPort
	);

	void start();

private:
	void work();

private:
	const char *m_broker;
	double m_spread;
	double m_quantity;
	char m_publicationURI[64];
	char m_subscriptionURI[64];
};


#endif //ARBITO_IBOFFICE_H
