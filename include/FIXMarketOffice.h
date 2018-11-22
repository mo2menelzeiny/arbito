
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

// SPDLOG
#include <spdlog/spdlog.h>

class FIXMarketOffice {
public:
	FIXMarketOffice(
			const char *broker,
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

	void stop();

private:
	void work();

private:
	const char *m_broker;
	double m_quantity;
	char m_publicationURI[64];
	struct fix_field *m_MDRFields;
	struct fix_message m_MDRFixMessage{};
	FIXSession m_fixSession;
	std::thread m_worker;
	std::atomic_bool m_running;
};


#endif //ARBITO_FIXMARKETOFFICE_H
