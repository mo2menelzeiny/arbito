
#include "swissquote/MarketOffice.h"

namespace SWISSQUOTE {

	MarketOffice::MarketOffice(
			const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
			const shared_ptr<RingBuffer<MarketDataEvent>> &local_md_buffer,
			Recorder &recorder,
			BrokerConfig broker_config,
			double spread,
			double lot_size
	) : m_control_buffer(control_buffer),
	    m_local_md_buffer(local_md_buffer),
	    m_recorder(&recorder),
	    m_broker_config(broker_config),
	    m_spread(spread),
	    m_lot_size(lot_size) {
		swissquote_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &swissquote_fix_dialects[SWISSQUOTE_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));
	}

	void MarketOffice::start() {
		while (!connectToBroker()) {
			close(m_session->sockfd);
			SSL_free(m_session->ssl);
			ERR_free_strings();
			EVP_cleanup();
			swissquote_fix_session_free(m_session);
			this_thread::sleep_for(seconds(RECONNECT_DELAY_SEC));
		};

		auto poller = thread(&MarketOffice::poll, this);
		poller.detach();
	}

	bool MarketOffice::connectToBroker() {
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		m_ssl_ctx = SSL_CTX_new(TLSv1_client_method());
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv3);
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv2);
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);
		SSL_CTX_use_certificate_file(m_ssl_ctx, "./resources/cert.pem", SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "./resources/key.pem", SSL_FILETYPE_PEM);
		m_ssl = SSL_new(m_ssl_ctx);
		m_cfg.ssl = m_ssl;

		m_session = swissquote_fix_session_new(&m_cfg);
		if (!m_session) {
			m_recorder->systemEvent("MarketOffice: FIX Session FAILED", SE_TYPE_ERROR);
			return false;
		}

		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent) {
			m_recorder->systemEvent("MarketOffice: Host lookup FAILED", SE_TYPE_ERROR);
			return false;
		}

		char **ap;
		int saved_errno = 0;
		for (ap = host_ent->h_addr_list; *ap; ap++) {
			m_cfg.sockfd = socket(host_ent->h_addrtype, SOCK_STREAM, IPPROTO_TCP);

			if (m_cfg.sockfd < 0) {
				saved_errno = errno;
				continue;
			}

			struct sockaddr_in socket_address = (struct sockaddr_in) {
					.sin_family        = static_cast<sa_family_t>(host_ent->h_addrtype),
					.sin_port        = htons(static_cast<uint16_t>(m_broker_config.port)),
			};

			memcpy(&socket_address.sin_addr, *ap, static_cast<size_t>(host_ent->h_length));

			if (connect(m_cfg.sockfd, (const struct sockaddr *) &socket_address, sizeof(struct sockaddr_in)) < 0) {
				saved_errno = errno;
				close(m_cfg.sockfd);
				m_cfg.sockfd = -1;
				continue;
			}

			break;
		}

		if (m_cfg.sockfd < 0) {
			stringstream ss;
			ss << "MarketOffice: Socket connection FAILED " << saved_errno;
			const string &tmp = ss.str();
			const char *cstr = tmp.c_str();
			m_recorder->systemEvent(cstr, SE_TYPE_ERROR);
			return false;
		}

		if (swissquote_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0) {
			m_recorder->systemEvent("MarketOffice: Socket option FAILED", SE_TYPE_ERROR);
			return false;
		}

		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			m_recorder->systemEvent("MarketOffice: SSL FAILED", SE_TYPE_ERROR);
			return false;
		}

		fcntl(m_cfg.sockfd, F_SETFL, O_NONBLOCK);

		if (swissquote_fix_session_logon(m_session)) {
			m_recorder->systemEvent("MarketOffice: Logon FAILED", SE_TYPE_ERROR);
			return false;
		}
		m_recorder->systemEvent("MarketOffice: Logon OK", SE_TYPE_SUCCESS);

		if (swissquote_fix_session_marketdata_request(m_session)) {
			m_recorder->systemEvent("MarketOffice: Market data request FAILED", SE_TYPE_ERROR);
			return false;
		}
		m_recorder->systemEvent("MarketOffice: Market data request OK", SE_TYPE_SUCCESS);

		return true;
	}

	void MarketOffice::poll() {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(1, &cpuset);
		pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
		pthread_setname_np(pthread_self(), "local");

		struct timespec curr{}, prev{};

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {
			clock_gettime(CLOCK_MONOTONIC, &curr);

			if ((curr.tv_sec - prev.tv_sec) > 0.1 * m_session->heartbtint) {
				prev = curr;

				if (!swissquote_fix_session_keepalive(m_session, &curr)) {
					m_session->active = false;
					continue;
				}
			}

			if (swissquote_fix_session_time_update(m_session)) {
				m_session->active = false;
				continue;
			}

			struct swissquote_fix_message *msg = nullptr;
			if (swissquote_fix_session_recv(m_session, &msg, SWISSQUOTE_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				if (!msg) {
					continue;
				}

				if (swissquote_fix_session_admin(m_session, msg)) {
					continue;
				}

				m_session->active = false;
			}

			switch (msg->type) {
				case SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
					if (m_spread < (swissquote_fix_get_field_at(msg, msg->nr_fields - 4)->float_value -
					                swissquote_fix_get_float(msg, swissquote_MDEntryPx, 0.0))
					    || m_lot_size > swissquote_fix_get_float(msg, swissquote_MDEntrySize, 0.0)
					    || m_lot_size > swissquote_fix_get_field_at(msg, msg->nr_fields - 3)->float_value) {
						continue;
					}

					auto data = MarketDataEvent();
					data.bid = swissquote_fix_get_float(msg, swissquote_MDEntryPx, 0.0);
					data.offer = swissquote_fix_get_field_at(msg, msg->nr_fields - 4)->float_value;
					data.timestamp_ms = (curr.tv_sec * 1000) + (curr.tv_nsec / 1000000);
					swissquote_fix_get_string(swissquote_fix_get_field(msg, swissquote_SendingTime), data.sending_time,
					                          64);

					try {
						auto next = m_local_md_buffer->tryNext();
						(*m_local_md_buffer)[next] = data;
						m_local_md_buffer->publish(next);
					} catch (InsufficientCapacityException &e) {
						m_recorder->systemEvent(
								"MarketOffice: Market buffer InsufficientCapacityException",
								SE_TYPE_ERROR
						);
					}

					try {
						auto next_record = m_recorder->m_local_records_buffer->tryNext();
						(*m_recorder->m_local_records_buffer)[next_record] = data;
						m_recorder->m_local_records_buffer->publish(next_record);
					} catch (InsufficientCapacityException &e) {
						m_recorder->systemEvent(
								"MarketOffice: Market records buffer InsufficientCapacityException",
								SE_TYPE_ERROR
						);
					}
				}
					continue;

				default:
					swissquote_fix_session_admin(m_session, msg);
					continue;
			}
		}

		try {
			auto next_pause = m_control_buffer->tryNext();
			(*m_control_buffer)[next_pause] = (ControlEvent) {
					.type = CET_PAUSE,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_pause);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent("MarketOffice: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}

		m_recorder->systemEvent("MarketOffice: Connection FAILED", SE_TYPE_ERROR);

		do {
			close(m_session->sockfd);
			SSL_free(m_session->ssl);
			ERR_free_strings();
			EVP_cleanup();
			swissquote_fix_session_free(m_session);
			this_thread::sleep_for(seconds(RECONNECT_DELAY_SEC));
		} while (!connectToBroker());

		auto poller = thread(&MarketOffice::poll, this);
		poller.detach();

		try {
			auto next_resume = m_control_buffer->tryNext();
			(*m_control_buffer)[next_resume] = (ControlEvent) {
					.type = CET_RESUME,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_resume);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent("MarketOffice: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}
	}
}