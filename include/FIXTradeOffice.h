
#ifndef ARBITO_FIXTRADEOFFICE_H
#define ARBITO_FIXTRADEOFFICE_H

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/NoOpIdleStrategy.h>
#include <FragmentAssembler.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "FIXSession.h"

class FIXTradeOffice {
public:
	FIXTradeOffice(
			const char *broker,
			double quantity,
			int subscriptionPort,
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat
	);

	void start();

private:
	void work();

private:
	const char *m_broker;
	double m_quantity;
	char m_subscriptionURI[64];
	struct fix_field *m_NOSSFields;
	struct fix_message m_NOSSFixMessage{};
	struct fix_field *m_NOSBFields;
	struct fix_message m_NOSBFixMessage{};
	FIXSession *m_fixSession;
};


#endif //ARBITO_FIXTRADEOFFICE_H
