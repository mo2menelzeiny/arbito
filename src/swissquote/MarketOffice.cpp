
#include "swissquote/MarketOffice.h"

namespace SWISSQUOTE {

	MarketOffice::MarketOffice(const char *m_host, int m_port, const char *username, const char *password,
	                                   const char *sender_comp_id, const char *target_comp_id, int heartbeat)
			: m_host(m_host), m_port(m_port) {
		// Session configurations
		swissquote_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &swissquote_fix_dialects[FIX_4_4];
		m_cfg.heartbtint = heartbeat;
		strncpy(m_cfg.username, username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, target_comp_id, ARRAY_SIZE(m_cfg.target_comp_id));

		// Disruptor
		m_taskscheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
		m_disruptor = std::make_shared<Disruptor::disruptor<Event>>(
				[]() { return Event(); },
				1024,
				m_taskscheduler,
				Disruptor::ProducerType::Single,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());
	}

	void MarketOffice::initialize() {
		// SSL options
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		m_ssl_ctx = SSL_CTX_new(TLS_client_method());
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv3);
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv2);
		SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);
		SSL_CTX_use_certificate_file(m_ssl_ctx, "./resources/cert.pem", SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "./resources/key.pem", SSL_FILETYPE_PEM);
		m_ssl = SSL_new(m_ssl_ctx);
		m_cfg.ssl = m_ssl;

		// Session object
		m_session = swissquote_fix_session_new(&m_cfg);
		if (!m_session) {
			fprintf(stderr, "FIX session cannot be created\n");
			return;
		}

		// Socket connection
		struct hostent *host_ent = gethostbyname(m_host);

		if (!host_ent)
			error("Unable to look up %s (%s)", m_host, hstrerror(h_errno));

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
					.sin_port        = htons(static_cast<uint16_t>(m_port)),
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

		if (m_cfg.sockfd < 0)
			error("Unable to connect to a socket (%s)", strerror(saved_errno));

		if (socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
			die("cannot set socket option TCP_NODELAY");

		// SSL connection
		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			fprintf(stderr, "SSL ERROR\n");
			return;
		}

		// SSL certificate
		X509 *server_cert;
		char *str;
		printf("SSL connection using %s\n", SSL_get_cipher (m_ssl));

		server_cert = SSL_get_peer_certificate(m_ssl);
		printf("Server certificate:\n");

		str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
		printf("\t subject: %s\n", str);
		OPENSSL_free(str);

		str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
		printf("\t issuer: %s\n", str);
		OPENSSL_free(str);

		// Session login
		if (swissquote_fix_session_logon(m_session)) {
			fprintf(stderr, "Client Logon FAILED\n");
			return;
		}
		fprintf(stdout, "Client Logon OK\n");

		// Market data request
		if (swissquote_fix_session_marketdata_request(m_session)) {
			fprintf(stderr, "Market data request FAILED\n");
			return;
		}
		fprintf(stdout, "Market data request OK\n");

		// Polling thread loop
		m_polling_thread = std::thread(&MarketOffice::poll, this);
		m_polling_thread.detach();
	}

	void MarketOffice::start() {
		m_taskscheduler->start();
		m_disruptor->start();
		initialize();
	}

	void MarketOffice::poll() {
		struct timespec cur{}, prev{};
		__time_t diff;

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &cur);

			diff = (cur.tv_sec - prev.tv_sec);

			if (diff > 0.1 * m_session->heartbtint) {
				prev = cur;

				if (!swissquote_fix_session_keepalive(m_session, &cur)) {
					fprintf(stderr, "Session keep alive FAILED\n");
					break;
				}
			}

			if (swissquote_fix_session_time_update(m_session)) {
				fprintf(stderr, "Session time update FAILED\n");
				break;
			}

			struct swissquote_fix_message *msg = nullptr;
			if (swissquote_fix_session_recv(m_session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				if (!msg) {
					continue;
				}

				if (swissquote_fix_session_admin(m_session, msg)) {
					continue;
				}
			}

			switch (msg->type) {
				case SWISSQUOTE_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH:
					onData(msg);
					continue;
				case SWISSQUOTE_FIX_MSG_TYPE_HEARTBEAT:
					onData(msg);
					continue;
				default:
					continue;
			}
		}

		// Reconnection condition
		if (m_session->active) {
			std::this_thread::sleep_for(std::chrono::seconds(30));
			fprintf(stdout, "Market data client reconnecting..\n");
			SSL_shutdown(m_cfg.ssl);
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			swissquote_fix_session_free(m_session);
			initialize();
		}
	}

	void MarketOffice::onData(swissquote_fix_message *msg) {
		fprintmsg(stdout, msg);

		auto next_sequence = m_disruptor->ringBuffer()->next();

		(*m_disruptor->ringBuffer())[next_sequence] = (Event) {
				.event_type = MARKET_DATA,
				.broker = BROKER_SWISSQUOTE,
				// First 2 fields are for bid - 0
				.bid = swissquote_fix_get_float(msg, MDEntryPx, 0.0),
				.bid_qty = swissquote_fix_get_float(msg, MDEntrySize, 0.0),
				// last 2 fields are for offer - 1
				.offer = swissquote_fix_get_field_at(msg, msg->nr_fields - 2)->float_value,
				.offer_qty = swissquote_fix_get_field_at(msg, msg->nr_fields - 1)->float_value
		};

		m_disruptor->ringBuffer()->publish(next_sequence);

	}
}