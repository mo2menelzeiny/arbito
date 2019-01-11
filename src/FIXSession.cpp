
#include "FIXSession.h"

FIXSession::FIXSession(
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat,
		bool sslEnabled
) : m_username(username),
    m_password(password),
    m_sender(sender),
    m_target(target),
    m_heartbeat(heartbeat),
    m_fixSocket(host, port, sslEnabled) {
}

void FIXSession::initiate() {
	m_fixSocket.initiate();

	fix_session_cfg_init(&m_cfg);

	m_cfg.sockfd = m_fixSocket.socketFD();
	m_cfg.ssl = m_fixSocket.ssl();

	m_cfg.dialect = &fix_dialects[FIX_4_4];
	m_cfg.heartbtint = m_heartbeat;

	strncpy(m_cfg.username, m_username, ARRAY_SIZE(m_cfg.username));
	strncpy(m_cfg.password, m_password, ARRAY_SIZE(m_cfg.password));
	strncpy(m_cfg.sender_comp_id, m_sender, ARRAY_SIZE(m_cfg.sender_comp_id));
	strncpy(m_cfg.target_comp_id, m_target, ARRAY_SIZE(m_cfg.target_comp_id));

	m_session = fix_session_new(&m_cfg);

	if (fix_session_logon(m_session)) {
		throw std::runtime_error("Session login FAILED");
	}

	clock_gettime(CLOCK_MONOTONIC, &m_lastTimespec);
}

void FIXSession::terminate() {
	m_fixSocket.terminate();

	if (m_session == nullptr) {
		return;
	}

	m_session->active = false;
	fix_session_free(m_session);
}


