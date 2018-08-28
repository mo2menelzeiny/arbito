
#include "lmax/TradeOffice.h"

namespace LMAX {

	TradeOffice::TradeOffice() = default;

	TradeOffice::TradeOffice(Recorder &recorder, Messenger &messenger,
	                         const std::shared_ptr<Disruptor::RingBuffer<ArbitrageDataEvent>> &arbitrage_data_ringbuffer,
	                         MessengerConfig messenger_config, BrokerConfig broker_config, double diff_open,
	                         double diff_close, double lot_size)
			: m_recorder(&recorder), m_messenger(&messenger), m_messenger_config(messenger_config),
			  m_broker_config(broker_config), m_arbitrage_data_ringbuffer(arbitrage_data_ringbuffer),
			  m_diff_open(diff_open), m_diff_close(diff_close), m_lot_size(lot_size) {
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));

		srand(static_cast<unsigned int>(time(nullptr)));

		m_atomic_buffer.wrap(m_buffer, LMAX_TO_MESSENGER_BUFFER);
	}

	void TradeOffice::start() {
		initMessengerClient();
		initBrokerClient();
	}

	void TradeOffice::initMessengerClient() {
		std::int64_t publication_id = m_messenger->aeronClient()->addPublication(m_messenger_config.pub_channel,
		                                                                         m_messenger_config.stream_id);
		std::int64_t subscription_id = m_messenger->aeronClient()->addSubscription(m_messenger_config.sub_channel,
		                                                                           m_messenger_config.stream_id);

		m_messenger_pub = m_messenger->aeronClient()->findPublication(publication_id);
		while (!m_messenger_pub) {
			std::this_thread::yield();
			m_messenger_pub = m_messenger->aeronClient()->findPublication(publication_id);
		}
		printf("TradeOffice: publication found!\n");

		m_messenger_sub = m_messenger->aeronClient()->findSubscription(subscription_id);
		while (!m_messenger_sub) {
			std::this_thread::yield();
			m_messenger_sub = m_messenger->aeronClient()->findSubscription(subscription_id);
		}
		printf("TradeOffice: subscription found!\n");

		m_recorder->recordSystem("TradeOffice: messenger channel OK", SYSTEM_RECORD_TYPE_SUCCESS);
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
		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent)
			error("Unable to look up %s (%s)", m_broker_config.host, hstrerror(h_errno));

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

		fcntl(m_cfg.sockfd, F_SETFL, O_NONBLOCK);

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
		time_t counter = time(nullptr);
		time_t timeout = LMAX_DELAY_SECONDS;

		auto arbitrage_data_poller = m_arbitrage_data_ringbuffer->newPoller();
		m_arbitrage_data_ringbuffer->addGatingSequences({arbitrage_data_poller->sequence()});
		auto arbitrage_data_handler = [&](ArbitrageDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
			if (check_timeout && ((time(nullptr) - counter) < timeout)) {
				return true;
			}

			check_timeout = false;

			if (0 == m_orders_count) {
				m_open_state = NO_DEALS;
			}

			struct lmax_fix_message *response;

			switch (m_open_state) {
				case CURRENT_DIFF_1: {
					if (m_orders_count < LMAX_MAX_DEALS && data.bid2_minus_offer1() >= m_diff_open) {
						if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.bid2_minus_offer1(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Buy order OK\n");
						++m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}

					if (data.bid1_minus_offer2() >= m_diff_close) {
						if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.bid1_minus_offer2(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Sell order OK\n");
						--m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}
				}
					break;

				case CURRENT_DIFF_2: {
					if (m_orders_count < LMAX_MAX_DEALS && data.bid1_minus_offer2() >= m_diff_open) {
						if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.bid1_minus_offer2(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_OPEN);

						fprintf(stdout, "Sell order OK\n");
						++m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}

					if (data.bid2_minus_offer1() >= m_diff_close) {
						if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.bid2_minus_offer1(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Buy order OK\n");
						--m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}
				}
					break;

				case NO_DEALS: {
					if (data.bid1_minus_offer2() >= m_diff_open) {
						if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
							fprintf(stderr, "Sell order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.bid, ORDER_RECORD_TYPE_SELL, data.bid1_minus_offer2(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_INIT);

						fprintf(stdout, "Sell order OK\n");
						m_open_state = CURRENT_DIFF_2;
						++m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}

					if (data.bid2_minus_offer1() >= m_diff_open) {
						if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
							fprintf(stderr, "Buy order FAILED\n");
							return true;
						};

						m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
						                        data.l1.offer, ORDER_RECORD_TYPE_BUY, data.bid2_minus_offer1(),
						                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_INIT);

						fprintf(stdout, "Buy order OK\n");
						m_open_state = CURRENT_DIFF_1;
						++m_orders_count;
						counter = time(nullptr);
						check_timeout = true;
						return true;
					}
				}
					break;
			}

			return true;
		};

		aeron::BusySpinIdleStrategy messengerIdleStrategy;
		sbe::MessageHeader sbe_msg_header;
		sbe::TradeConfirm sbe_trade_confirm;
		aeron::FragmentAssembler messengerAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
		                                                aeron::index_t length, const aeron::Header &header) {
			sbe_msg_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, LMAX_TO_MESSENGER_BUFFER);
			sbe_trade_confirm.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
			                                sbe::MessageHeader::encodedLength(), sbe_msg_header.blockLength(),
			                                sbe_msg_header.version(), LMAX_TO_MESSENGER_BUFFER);

			if (m_orders_count > sbe_trade_confirm.ordersCount()) {
				struct lmax_fix_message *response;

				switch (m_open_state) {
					case CURRENT_DIFF_1:
						if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
							fprintf(stderr, "Correction sell order FAILED\n");
							return;
						};

						m_recorder->recordOrder(0, 0, ORDER_RECORD_TYPE_SELL, 0, ORDER_TRIGGER_TYPE_CORRECTION,
						                        ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Correction sell order OK\n");
						--m_orders_count;
						return;

					case CURRENT_DIFF_2:
						if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
							fprintf(stderr, "Correction buy order FAILED\n");
							return;
						};

						m_recorder->recordOrder(0, 0, ORDER_RECORD_TYPE_BUY, 0, ORDER_TRIGGER_TYPE_CORRECTION,
						                        ORDER_RECORD_STATE_CLOSE);

						fprintf(stdout, "Correction buy order OK\n");
						--m_orders_count;
						return;

					case NO_DEALS:
						return;
				}
			}
		});

		struct timespec cur{}, prev{}, confirm_timer{};
		clock_gettime(CLOCK_MONOTONIC, &prev);
		clock_gettime(CLOCK_MONOTONIC, &confirm_timer);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &cur);

			if ((cur.tv_sec - prev.tv_sec) > (0.1 * m_session->heartbtint)) {
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

			if (30 < (cur.tv_sec - confirm_timer.tv_sec)) {
				confirm_timer = cur;
				confirmOrders();
			}

			arbitrage_data_poller->poll(arbitrage_data_handler);

			messengerIdleStrategy.idle(m_messenger_sub->poll(messengerAssembler.handler(), 10));

			struct lmax_fix_message *msg;
			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				continue;
			}

			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_TEST_REQUEST:
					lmax_fix_session_admin(m_session, msg);
					continue;
				default:
					continue;
			}

		}

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

	void TradeOffice::confirmOrders() {
		sbe::MessageHeader sbe_msg_header;
		sbe::TradeConfirm sbe_trade_confirm;
		sbe_msg_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, LMAX_TO_MESSENGER_BUFFER)
				.blockLength(sbe::TradeConfirm::sbeBlockLength())
				.templateId(sbe::TradeConfirm::sbeTemplateId())
				.schemaId(sbe::TradeConfirm::sbeSchemaId())
				.version(sbe::TradeConfirm::sbeSchemaVersion());

		sbe_trade_confirm.wrapForEncode(reinterpret_cast<char *>(m_buffer), sbe::MessageHeader::encodedLength(),
		                                LMAX_TO_MESSENGER_BUFFER)
				.ordersCount(static_cast<const uint8_t>(m_orders_count));

		aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_trade_confirm.encodedLength();
		std::int64_t result;
		do {
			result = m_messenger_pub->offer(m_atomic_buffer, 0, len);
		} while (result < -1);
	}
}
