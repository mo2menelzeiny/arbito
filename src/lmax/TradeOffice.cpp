
#include "lmax/TradeOffice.h"

namespace LMAX {

	TradeOffice::TradeOffice(const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
	                         const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
	                         Recorder &recorder, Messenger &messenger, BrokerConfig broker_config, double lot_size)
			: m_business_buffer(business_buffer), m_trade_buffer(trade_buffer), m_recorder(&recorder),
			  m_messenger(&messenger), m_broker_config(broker_config), m_lot_size(lot_size) {
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));
	}

	void TradeOffice::start() {
		initBrokerClient();
	}

	void TradeOffice::initBrokerClient() {
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
			m_recorder->recordSystem("TradeOffice: FIX Session FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "TradeOffice: FIX session cannot be created\n");
			return;
		}

		// Socket connection
		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent) {
			m_recorder->recordSystem("TradeOffice: Host lookup FAILED", SYSTEM_RECORD_TYPE_ERROR);
			error("TradeOffice: Unable to look up %s (%s)", m_broker_config.host, hstrerror(h_errno));
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
			m_recorder->recordSystem("TradeOffice: Socket connection FAILED", SYSTEM_RECORD_TYPE_ERROR);
			error("TradeOffice: Unable to connect to a socket (%s)", strerror(saved_errno));
		}

		if (lmax_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0) {
			m_recorder->recordSystem("TradeOffice: Socket option FAILED", SYSTEM_RECORD_TYPE_ERROR);
			die("TradeOffice: cannot set socket option TCP_NODELAY");
		}

		// SSL connection
		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			fprintf(stderr, "TradeOffice: SSL FAILED\n");
			m_recorder->recordSystem("TradeOffice: SSL FAILED", SYSTEM_RECORD_TYPE_ERROR);
			return;
		}

		// SSL certificate
		X509 *server_cert;
		char *str;
		server_cert = SSL_get_peer_certificate(m_ssl);
		str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
		OPENSSL_free(str);
		str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
		OPENSSL_free(str);

		fcntl(m_cfg.sockfd, F_SETFL, O_NONBLOCK);

		// Session login
		int logon_result;
		do {

			logon_result = lmax_fix_session_logon(m_session);

			if (logon_result) {
				m_recorder->recordSystem("TradeOffice: broker client logon FAILED", SYSTEM_RECORD_TYPE_ERROR);
				fprintf(stderr, "TradeOffice: Client Logon FAILED\n");
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}

		} while (logon_result);

		m_poller = std::thread(&TradeOffice::poll, this);
		m_poller.detach();

	}

	void TradeOffice::poll() {
		auto business_poller = m_business_buffer->newPoller();
		m_business_buffer->addGatingSequences({business_poller->sequence()});
		auto business_handler = [&](BusinessEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
			char id[16];
			sprintf(id, "%i", data.clOrdId);
			struct lmax_fix_field fields[] = {
					LMAX_FIX_STRING_FIELD(lmax_ClOrdID, id),
					LMAX_FIX_STRING_FIELD(lmax_SecurityID, "4001"), // EUR/USD
					LMAX_FIX_STRING_FIELD(lmax_SecurityIDSource, "8"),
					LMAX_FIX_CHAR_FIELD(lmax_Side, data.side),
					LMAX_FIX_STRING_FIELD(lmax_TransactTime, m_session->str_now),
					LMAX_FIX_FLOAT_FIELD(lmax_OrderQty, m_lot_size),
					LMAX_FIX_CHAR_FIELD(lmax_OrdType, '1') // Market best
			};

			struct lmax_fix_message order_msg{};
			order_msg.type = LMAX_FIX_MSG_TYPE_NEW_ORDER_SINGLE;
			order_msg.fields = fields;
			order_msg.nr_fields = ARRAY_SIZE(fields);
			lmax_fix_session_send(m_session, &order_msg, 0);
			m_session->active = true;
			return true;
		};

		struct timespec curr{}, prev{};
		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			clock_gettime(CLOCK_MONOTONIC, &curr);

			if ((curr.tv_sec - prev.tv_sec) > (0.1 * m_session->heartbtint)) {
				prev = curr;

				if (!lmax_fix_session_keepalive(m_session, &curr)) {
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				break;
			}

			business_poller->poll(business_handler);

			struct lmax_fix_message *msg;

			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				continue;
			}

			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_EXECUTION_REPORT:
					if (lmax_fix_get_field(msg, lmax_ExecType)->string_value[0] == 'F') {
						auto next = m_trade_buffer->next();
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_OrderID), (*m_trade_buffer)[next].orderId, 64);
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_ClOrdID), (*m_trade_buffer)[next].clOrdId, 64);
						(*m_trade_buffer)[next].side = lmax_fix_get_field(msg, lmax_Side)->string_value[0];
						(*m_trade_buffer)[next].avgPx = lmax_fix_get_field(msg, lmax_AvgPx)->float_value;
						(*m_trade_buffer)[next].timestamp_us = (curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L);
						m_trade_buffer->publish(next);
					}
					continue;

				case LMAX_FIX_MSG_TYPE_TEST_REQUEST:
					lmax_fix_session_admin(m_session, msg);
					continue;

				default:
					continue;
			}

		}

		if (m_session->active) {
			m_recorder->recordSystem("TradeOffice: Session FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "TradeOffice: Session FAILED\n");
			std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			initBrokerClient();
		}
	}
}
