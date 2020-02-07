
#include "NATSSession.h"

NATSSession::NATSSession() : m_host("localhost"), m_username(""), m_password(""), m_nc(nullptr) {}

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
		natsStatus (*connect_fn)(natsConnection **, natsOptions *)
) {
	natsOptions *opts = nullptr;

	if (create_opts_fn(&opts) != NATS_OK) {
		throw std::runtime_error("NATS Session Create options Failed");
	}

	std::string host = "nats://localhost:4222";

	if (strcmp(m_username, "") != 0 && strcmp(m_password, "") != 0) {
		host = "nats://"
		       + std::string(m_username) + ":" + std::string(m_password)
		       + "@" + std::string(m_host) + ":4222";
	}

	if (seturl_opts_fn(opts, host.c_str()) != NATS_OK) {
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

	natsOptions_Destroy(opts);
}

void NATSSession::terminate() {
	natsConnection_Close(m_nc);
	natsConnection_Destroy(m_nc);
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
