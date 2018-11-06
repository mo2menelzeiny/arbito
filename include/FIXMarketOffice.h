
#ifndef ARBITO_FIXMARKETOFFICE_H
#define ARBITO_FIXMARKETOFFICE_H

#include "FIXSession.h"

class FIXMarketOffice {
public:
	FIXMarketOffice(
			const char *broker,
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
	void poll();

private:
	const char *m_broker;
	struct fix_field *m_MDRFields;
	struct fix_message m_MDRFixMessage{};
	FIXSession *m_fixSession;
};


#endif //ARBITO_FIXMARKETOFFICE_H
