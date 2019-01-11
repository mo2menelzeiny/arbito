
#ifndef ARBITO_FIXSESSION_H
#define ARBITO_FIXSESSION_H

// C
#include <thread>
#include <functional>

// FIX
#include "libtrading/proto/fix_common.h"

extern "C" {
#include "libtrading/proto/test.h"
};


//Domain
#include "FIXSocket.h"

typedef std::function<void(struct fix_message *msg)> OnMessageHandler;

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
			bool sslEnabled
	);

	inline bool isActive() const {
		return m_session == nullptr ? false : m_session->active;
	}

	inline char *strNow() const {
		return m_session->str_now;
	}

	void initiate();

	void terminate();

	inline void send(struct fix_message *msg) {
		fix_session_send(m_session, msg, 0);
	}

	template<typename F>
	inline void poll(F &&onMessageHandler) {
		struct timespec currentTimespec{};
		clock_gettime(CLOCK_MONOTONIC, &currentTimespec);

		if ((currentTimespec.tv_sec - m_lastTimespec.tv_sec) > 0.1 * m_session->heartbtint) {
			m_lastTimespec = currentTimespec;

			if (!fix_session_keepalive(m_session, &currentTimespec)) {
				m_session->active = false;
				return;
			}
		}

		if (fix_session_time_update(m_session)) {
			m_session->active = false;
			return;
		}

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
	FIXSocket m_fixSocket;
	struct fix_session_cfg m_cfg;
	struct fix_session *m_session = nullptr;
	struct timespec m_lastTimespec{};
};


#endif //ARBITO_FIXSESSION_H
