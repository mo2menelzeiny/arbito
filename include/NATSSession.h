
#ifndef ARBITO_NATSSESSION_H
#define ARBITO_NATSSESSION_H

// NATS
#include <nats/nats.h>

// C
#include <stdexcept>
#include <cstring>

class NATSSession {
public:
	NATSSession();

	NATSSession(const char *host, const char *username, const char *password);

	// TODO: pass all nats specific deps to constructor to mutate in tests
	void initiate(
			natsStatus (*create_opts_fn)(natsOptions **),
			natsStatus (*setAsap_opts_fn)(natsOptions *, bool),
			natsStatus (*seturl_opts_fn)(natsOptions *, const char *),
			natsStatus (*connect_fn)(natsConnection **, natsOptions *)
	);

	void terminate();

	void publishString(
			natsStatus (*pub_fn)(natsConnection *, const char *subj, const char *str),
			const char *channel,
			const char *msg
	);

private:
	const char *m_host;
	const char *m_username;
	const char *m_password;
	natsConnection *m_nc;
};


#endif //ARBITO_NATSSESSION_H
