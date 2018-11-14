
#ifndef ARBITO_FIXMARKETOFFICE_H
#define ARBITO_FIXMARKETOFFICE_H

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

class FIXMarketOffice {
public:
	FIXMarketOffice(
			const char *broker,
			double spread,
			double quantity,
			int publicationPort,
			const char *publicationHost,
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
	double m_spread;
	double m_quantity;
	char m_publicationURI[64];
	struct fix_field *m_MDRFields;
	struct fix_message m_MDRFixMessage{};
	FIXSession m_fixSession;
};


#endif //ARBITO_FIXMARKETOFFICE_H
