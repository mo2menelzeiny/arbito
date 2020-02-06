
#include "NATSSession.h"

NATSSession::NATSSession() : m_host("nats://localhost:4222"), m_username(""), m_password(""), m_nc(nullptr) {}

NATSSession::NATSSession(const char *host, const char *username, const char *password)
		: m_host(host),
		  m_username(username),
		  m_password(password),
		  m_nc(nullptr) {

}

void NATSSession::initiate(
		natsStatus (*create_opts_fn)(natsOptions **),
		natsStatus (*setAsap_opts_fn)(natsOptions *, bool),
		natsStatus (*seturl_opts_fn)(natsOptions *, const char *),
		natsStatus (*connect_fn)(natsConnection **, natsOptions *),
		void (*destroy_opts_fn)(natsOptions *)
) {
	natsOptions *opts = nullptr;


	if (create_opts_fn(&opts) != NATS_OK) {
		throw std::runtime_error("NATS Session Create options Failed");
	}

	if (seturl_opts_fn(opts, m_host) != NATS_OK) {
		throw std::runtime_error("NATS Session set url options Failed");
	}

	if (setAsap_opts_fn(opts, true) != NATS_OK) {
		throw std::runtime_error("NATS Session set asap options Failed");
	}

	natsStatus s;
	s = connect_fn(&m_nc, opts);
	if (s != NATS_OK) {
		throw std::runtime_error("NATS Session connect Failed");
	}

	destroy_opts_fn(opts);
}

void NATSSession::terminate(
		void (*conn_close_fn)(natsConnection *),
		void (*conn_destroy_fn)(natsConnection *)
) {
	conn_close_fn(m_nc);
	conn_destroy_fn(m_nc);
}

void NATSSession::publishString(
		natsStatus (*pub_fn)(natsConnection *, const char *, const char *),
		const char *channel,
		const char *msg
) {
	natsStatus s;
	s = pub_fn(m_nc, channel, msg);
	if (s != NATS_OK) {
		throw std::runtime_error("NATS Session publish failed");
	}
}
