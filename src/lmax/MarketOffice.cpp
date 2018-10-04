
#include "lmax/MarketOffice.h"

namespace LMAX {

	using namespace std::chrono;

	MarketOffice::MarketOffice(
			const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
			Recorder &recorder, BrokerConfig broker_config, double spread, double lot_size)
			: m_control_buffer(control_buffer), m_local_md_buffer(local_md_buffer), m_recorder(&recorder),
			  m_broker_config(broker_config), m_spread(spread), m_lot_size(lot_size) {
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));
	}

	void MarketOffice::start() {
		connectToBroker();
	}

	void MarketOffice::connectToBroker() {
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

		// Session object
		m_session = lmax_fix_session_new(&m_cfg);
		if (!m_session) {
			m_recorder->recordSystemMessage("MarketOffice: FIX Session FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: FIX session cannot be created\n");
			return;
		}

		// Socket connection
		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent) {
			m_recorder->recordSystemMessage("MarketOffice: Host lookup FAILED", SYSTEM_RECORD_TYPE_ERROR);
			error("MarketOffice: Unable to look up %s (%s)", m_broker_config.host, hstrerror(h_errno));
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
			m_recorder->recordSystemMessage("MarketOffice: Socket connection FAILED", SYSTEM_RECORD_TYPE_ERROR);
			error("MarketOffice: Unable to connect to a socket (%s)", strerror(saved_errno));
		}

		if (lmax_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0) {
			m_recorder->recordSystemMessage("MarketOffice: Socket option FAILED", SYSTEM_RECORD_TYPE_ERROR);
			die("MarketOffice: cannot set socket option TCP_NODELAY");
		}

		// SSL connection
		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			m_recorder->recordSystemMessage("MarketOffice: SSL FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: SSL FAILED\n");
			return;
		}

		fcntl(m_cfg.sockfd, F_SETFL, O_NONBLOCK);

		if (lmax_fix_session_logon(m_session)) {
			m_recorder->recordSystemMessage("MarketOffice: Logon FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: Logon FAILED\n");
		}
		m_recorder->recordSystemMessage("MarketOffice: Logon OK", SYSTEM_RECORD_TYPE_SUCCESS);
		fprintf(stdout, "MarketOffice: Logon OK\n");

		if (lmax_fix_session_marketdata_request(m_session)) {
			m_recorder->recordSystemMessage("MarketOffice: Market data request FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: Market data request FAILED\n");
		}
		m_recorder->recordSystemMessage("MarketOffice: Market data request OK", SYSTEM_RECORD_TYPE_SUCCESS);
		fprintf(stdout, "MarketOffice: Market data request OK\n");

		m_poller = std::thread(&MarketOffice::poll, this);
		m_poller.detach();
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

				if (!lmax_fix_session_keepalive(m_session, &curr)) {
					m_session->active = false;
					continue;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				m_session->active = false;
				continue;
			}

			struct lmax_fix_message *msg = nullptr;
			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				if (!msg) {
					continue;
				}

				if (lmax_fix_session_admin(m_session, msg)) {
					continue;
				}

				m_session->active = false;
			}

			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
					if (m_spread < (lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value -
					                lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0))
					    || m_lot_size > lmax_fix_get_float(msg, lmax_MDEntrySize, 0.0)
					    || m_lot_size > lmax_fix_get_field_at(msg, msg->nr_fields - 1)->float_value) {
						continue;
					}

					auto data = (MarketDataEvent) {
							.bid = lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0),
							.offer = lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value,
							.timestamp_ms  = (curr.tv_sec * 1000) + (curr.tv_nsec / 1000000)
					};

					try {
						auto next = m_local_md_buffer->next();
						(*m_local_md_buffer)[next] = data;
						m_local_md_buffer->publish(next);
					} catch (Disruptor::InsufficientCapacityException &e) {
						fprintf(stderr, "MarketOffice: Market buffer InsufficientCapacityException\n");
					}
				}
					continue;

				case LMAX_FIX_MSG_TYPE_TEST_REQUEST:
					lmax_fix_session_admin(m_session, msg);
					continue;

				default:
					continue;
			}
		}

		auto next_pause = m_control_buffer->next();
		(*m_control_buffer)[next_pause] = (ControlEvent) {
				.source = CES_MARKET_OFFICE,
				.type = CET_PAUSE,
				.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
		};
		m_control_buffer->publish(next_pause);

		m_recorder->recordSystemMessage("MarketOffice: Connection FAILED", SYSTEM_RECORD_TYPE_ERROR);
		fprintf(stderr, "MarketOffice: Connection FAILED\n");

		std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));

		SSL_free(m_cfg.ssl);
		ERR_free_strings();
		EVP_cleanup();
		lmax_fix_session_free(m_session);

		connectToBroker();

		auto next_resume = m_control_buffer->next();
		(*m_control_buffer)[next_resume] = (ControlEvent) {
				.source = CES_MARKET_OFFICE,
				.type = CET_RESUME,
				.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
		};
		m_control_buffer->publish(next_resume);
	}
}