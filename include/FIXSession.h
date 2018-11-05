
#ifndef ARBITO_FIXSESSION_H
#define ARBITO_FIXSESSION_H

// C
#include <thread>

// FIX
#include <libtrading/proto/fix_common.h>
#include <functional>

//Domain
#include "FIXSocket.h"


typedef std::function<void(const std::exception &ex)> OnErrorHandler;

typedef std::function<void(const fix_session *session)> OnStartHandler;

typedef std::function<void(struct fix_message *msg)> OnMessageHandler;

typedef std::function<void(struct fix_session *session)> DoWorkHandler;

class FIXSession {
public:
	FIXSession(
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat,
			const OnErrorHandler &onErrorHandler,
			const OnStartHandler &onStartHandler
	);

	inline fix_session *session() const {
		return m_session;
	}

	void initiate();

	void terminate();

	template<typename A, typename B>
	inline void poll(A &&doWorkHandler, B &&onMessageHandler) {
		if (!m_session->active) {
			this->m_onErrorHandler(std::runtime_error("Session FAILED"));
			this->initiate();
			return;
		}

		clock_gettime(CLOCK_MONOTONIC, &currentTimespec);

		if ((currentTimespec.tv_sec - previousTimespec.tv_sec) > 0.1 * m_session->heartbtint) {
			previousTimespec = currentTimespec;

			if (!fix_session_keepalive(m_session, &currentTimespec)) {
				m_session->active = false;
				return;
			}
		}

		if (fix_session_time_update(m_session)) {
			m_session->active = false;
			return;
		}

		doWorkHandler(m_session);

		struct fix_message *msg = nullptr;
		if (fix_session_recv(m_session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
			if (!msg) {
				return;
			}

			if (fix_session_admin(m_session, msg)) {
				return;
			}

			m_session->active = false;
			return;
		}

		switch (msg->type) {
			case FIX_MSG_TYPE_HEARTBEAT:
			case FIX_MSG_TYPE_TEST_REQUEST:
			case FIX_MSG_TYPE_RESEND_REQUEST:
			case FIX_MSG_TYPE_SEQUENCE_RESET:
				fix_session_admin(m_session, msg);
				break;
			default:
				onMessageHandler(msg);
		}

	}

private:
	const char *m_username;
	const char *m_password;
	const char *m_sender;
	const char *m_target;
	int m_heartbeat;
	OnErrorHandler m_onErrorHandler;
	OnStartHandler m_onStartHandler;
	FIXSocket m_fixSocket;
	struct fix_session_cfg m_cfg;
	struct fix_session *m_session;
	struct timespec currentTimespec{};
	struct timespec previousTimespec{};
};


#endif //ARBITO_FIXSESSION_H
