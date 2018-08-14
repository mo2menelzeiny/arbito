
#include "lmax/TradeOffice.h"

namespace LMAX {

	TradeOffice::TradeOffice(const std::shared_ptr<Recorder> &recorder, const std::shared_ptr<Messenger> &messenger,
	                         const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> &arbitrage_data_disruptor,
	                         const char *m_host, int m_port, const char *username, const char *password,
	                         const char *sender_comp_id,
	                         const char *target_comp_id, int heartbeat, double diff_open, double diff_close,
	                         double bid_lot_size,
	                         double offer_lot_size)
			: m_recorder(recorder), m_messenger(messenger), m_arbitrage_data_disruptor(arbitrage_data_disruptor),
			  m_host(m_host),
			  m_port(m_port), m_diff_open(diff_open), m_diff_close(diff_close), m_bid_lot_size(bid_lot_size),
			  m_offer_lot_size(offer_lot_size) {
		// Session configurations
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = heartbeat;
		strncpy(m_cfg.username, username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, target_comp_id, ARRAY_SIZE(m_cfg.target_comp_id));
		srand(static_cast<unsigned int>(time(nullptr)));
	}

	void TradeOffice::start() {
		initBrokerClient();
	}

	void TradeOffice::initBrokerClient() {
		printf("Initializing trade office broker client..\n");
		// SSL options
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

		if (lmax_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
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
		if (lmax_fix_session_logon(m_session)) {
			fprintf(stderr, "TradeOffice: Client Logon FAILED\n");
			return;
		}
		fprintf(stdout, "TradeOffice: Client Logon OK\n");

		// Polling thread loop
		poller = std::thread(&TradeOffice::poll, this);
		poller.detach();

		m_recorder->recordSystem("TradeOffice: broker client OK", SYSTEM_RECORD_TYPE_SUCCESS);
	}

	void TradeOffice::poll() {
		bool check_timeout = false;
		time_t counter = time(0);
		time_t timeout = LMAX_DELAY_SECONDS;
		auto arbitrage_data_poller = m_arbitrage_data_disruptor->ringBuffer()->newPoller();
		auto arbitrage_data_handler = [&](ArbitrageDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {

			if (check_timeout && (time(0) - counter < timeout)) {
				return false;
			}
			check_timeout = false;

			if (!m_deals_count) {
				m_open_state = NO_DEALS;
			}
			// current difference 1 -> offer1 - bid2
			// current difference 2 -> offer2 - bid1
			switch (m_open_state) {
				case OFFER1_MINUS_BID2: {

					if (m_deals_count < LMAX_MAX_DEALS && data.offer1_minus_bid2() >= m_diff_open) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '1', &m_offer_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							counter = time(0);
							return false;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.offer1_minus_bid2(),
						                        ORDER_TRIGGER_TYPE_OFFER1_MINUS_BID2, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Buy order OK\n");
						++m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}

					if (data.offer2_minus_bid1() >= m_diff_close) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '2', &m_bid_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							counter = time(0);
							return false;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.offer2_minus_bid1(),
						                        ORDER_TRIGGER_TYPE_OFFER2_MINUS_BID1, ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Sell order OK\n");
						--m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}
				}
					break;
				case OFFER2_MINUS_BID1: {

					if (m_deals_count < LMAX_MAX_DEALS && data.offer2_minus_bid1() >= m_diff_open) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '2', &m_bid_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							counter = time(0);
							return false;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.offer2_minus_bid1(),
						                        ORDER_TRIGGER_TYPE_OFFER2_MINUS_BID1, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Sell order OK\n");
						++m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}

					if (data.offer1_minus_bid2() >= m_diff_close) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '1', &m_offer_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							counter = time(0);
							return false;
						};


						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.offer1_minus_bid2(),
						                        ORDER_TRIGGER_TYPE_OFFER1_MINUS_BID2, ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Buy order OK\n");
						--m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}
				}
					break;

				case NO_DEALS: {
					if (data.offer1_minus_bid2() >= m_diff_open) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '1', &m_offer_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							counter = time(0);
							return false;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.offer1_minus_bid2(),
						                        ORDER_TRIGGER_TYPE_OFFER1_MINUS_BID2, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Buy order OK\n");
						m_open_state = OFFER1_MINUS_BID2;
						++m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}

					if (data.offer2_minus_bid1() >= m_diff_open) {
						struct lmax_fix_message *response = nullptr;
						if (lmax_fix_session_new_order_single(m_session, '2', &m_bid_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							counter = time(0);
							return false;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.offer2_minus_bid1(),
						                        ORDER_TRIGGER_TYPE_OFFER2_MINUS_BID1, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Sell order OK\n");

						m_open_state = OFFER2_MINUS_BID1;
						++m_deals_count;
						counter = time(0);
						check_timeout = true;
						return false;
					}
				}
					break;
			}

			return false;

		};

		struct timespec cur{}, prev{};
		__time_t diff;

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &cur);

			diff = (cur.tv_sec - prev.tv_sec);

			if (diff > 0.1 * m_session->heartbtint) {
				prev = cur;

				if (!lmax_fix_session_keepalive(m_session, &cur)) {
					fprintf(stderr, "TradeOffice: Session keep alive FAILED\n");
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				fprintf(stderr, "TradeOffice: Session time update FAILED\n");
				break;
			}

			struct lmax_fix_message *msg = nullptr;
			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				continue;
			}

			arbitrage_data_poller->poll(arbitrage_data_handler);
		}

		// Reconnection condition
		if (m_session->active) {
			fprintf(stdout, "Trade office reconnecting..\n");
			m_recorder->recordSystem("TradeOffice: broker client FAILED", SYSTEM_RECORD_TYPE_ERROR);
			std::this_thread::sleep_for(std::chrono::seconds(30));
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			initBrokerClient();
		}
	}
}
