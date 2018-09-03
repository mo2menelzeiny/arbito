
#include "lmax/TradeOffice.h"

namespace LMAX {

	TradeOffice::TradeOffice() = default;

	TradeOffice::TradeOffice(Recorder &recorder, Messenger &messenger,
	                         const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_ringbuffer,
	                         const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &remote_md_ringbuff,
	                         MessengerConfig messenger_config, BrokerConfig broker_config, double diff_open,
	                         double diff_close, double lot_size)
			: m_recorder(&recorder), m_messenger(&messenger), m_local_md_ringbuffer(local_md_ringbuffer),
			  m_remote_md_ringbuffer(remote_md_ringbuff), m_messenger_config(messenger_config),
			  m_broker_config(broker_config), m_diff_open(diff_open), m_diff_close(diff_close), m_lot_size(lot_size) {
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
		bool close_delay_check = false;
		time_t close_delay_start = time(nullptr);
		time_t close_delay = LMAX_DELAY_SECONDS;
		std::deque<MarketDataEvent> local_md;
		struct timespec confirm_delay_start{};
		bool confirm_delay_check = false;

		auto local_md_poller = m_local_md_ringbuffer->newPoller();
		m_local_md_ringbuffer->addGatingSequences({local_md_poller->sequence()});
		auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
			local_md.push_front(data);
		};

		auto remote_md_poller = m_remote_md_ringbuffer->newPoller();
		m_remote_md_ringbuffer->addGatingSequences({remote_md_poller->sequence()});
		auto remote_md_handler = [&](MarketDataEvent &remote_md, std::int64_t sequence, bool endOfBatch) -> bool {

			if (0 == m_orders_count) {
				m_open_state = NO_DEALS;
			}

			for (int i = 0; i < local_md.size(); i++) {
				struct lmax_fix_message *response;
				switch (m_open_state) {
					case CURRENT_DIFF_1:
						if (m_orders_count < LMAX_MAX_DEALS &&
						    (remote_md.bid - local_md[i].offer) >= m_diff_open) {
							if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
								m_recorder->recordSystem("Buy order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Buy order FAILED\n");
								return true;
							};
							close_delay_start = time(nullptr);
							close_delay_check = true;
							clock_gettime(CLOCK_MONOTONIC, &confirm_delay_start);
							confirm_delay_check = true;
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].offer, ORDER_RECORD_TYPE_BUY,
							                        remote_md.bid - local_md[i].offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_OPEN);
							fprintf(stdout, "Buy order OK\n");
							local_md.pop_front();
							++m_orders_count;
							return true;
						}

						if (close_delay_check && ((time(nullptr) - close_delay_start) < close_delay)) {
							return true;
						}

						close_delay_check = false;

						if ((local_md[i].bid - remote_md.offer) >= m_diff_close) {
							if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
								m_recorder->recordSystem("Sell order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Sell order FAILED\n");
								return true;
							};
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].bid, ORDER_RECORD_TYPE_SELL,
							                        local_md[i].bid - remote_md.offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_CLOSE);
							fprintf(stdout, "Sell order OK\n");
							local_md.pop_front();
							--m_orders_count;
							return true;
						}

						break;

					case CURRENT_DIFF_2:
						if (m_orders_count < LMAX_MAX_DEALS &&
						    (local_md[i].bid - remote_md.offer) >= m_diff_open) {
							if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
								m_recorder->recordSystem("Sell order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Sell order FAILED\n");
								return true;
							};
							close_delay_start = time(nullptr);
							close_delay_check = true;
							clock_gettime(CLOCK_MONOTONIC, &confirm_delay_start);
							confirm_delay_check = true;
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].bid, ORDER_RECORD_TYPE_SELL,
							                        local_md[i].bid - remote_md.offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_OPEN);
							fprintf(stdout, "Sell order OK\n");
							local_md.pop_front();
							++m_orders_count;
							return true;
						}

						if (close_delay_check && ((time(nullptr) - close_delay_start) < close_delay)) {
							return true;
						}

						close_delay_check = false;

						if (remote_md.bid - local_md[i].offer >= m_diff_close) {
							if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
								m_recorder->recordSystem("Buy order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Buy order FAILED\n");
								return true;
							};
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].offer, ORDER_RECORD_TYPE_BUY,
							                        remote_md.bid - local_md[i].offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_CLOSE);
							fprintf(stdout, "Buy order OK\n");
							local_md.pop_front();
							--m_orders_count;
							return true;
						}

						break;

					case NO_DEALS:
						if ((local_md[i].bid - remote_md.offer) >= m_diff_open) {
							if (lmax_fix_session_new_order_single(m_session, '2', &m_lot_size, &response)) {
								m_recorder->recordSystem("Sell order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Sell order FAILED\n");
								return true;
							};
							close_delay_start = time(nullptr);
							close_delay_check = true;
							clock_gettime(CLOCK_MONOTONIC, &confirm_delay_start);
							confirm_delay_check = true;
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].bid, ORDER_RECORD_TYPE_SELL,
							                        local_md[i].bid - remote_md.offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_2, ORDER_RECORD_STATE_INIT);
							fprintf(stdout, "Sell order OK\n");
							local_md.pop_front();
							m_open_state = CURRENT_DIFF_2;
							++m_orders_count;
							return true;
						}

						if ((remote_md.bid - local_md[i].offer) >= m_diff_open) {
							if (lmax_fix_session_new_order_single(m_session, '1', &m_lot_size, &response)) {
								m_recorder->recordSystem("Buy order failed", SYSTEM_RECORD_TYPE_ERROR);
								fprintf(stderr, "Buy order FAILED\n");
								return true;
							};
							close_delay_start = time(nullptr);
							close_delay_check = true;
							clock_gettime(CLOCK_MONOTONIC, &confirm_delay_start);
							confirm_delay_check = true;
							m_recorder->recordOrder(lmax_fix_get_field(response, lmax_AvgPx)->float_value,
							                        local_md[i].offer, ORDER_RECORD_TYPE_BUY,
							                        remote_md.bid - local_md[i].offer,
							                        ORDER_TRIGGER_TYPE_CURRENT_DIFF_1, ORDER_RECORD_STATE_INIT);
							fprintf(stdout, "Buy order OK\n");
							local_md.pop_front();
							m_open_state = CURRENT_DIFF_1;
							++m_orders_count;
							return true;
						}

						break;
				}
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

			if (m_orders_count < sbe_trade_confirm.ordersCount()) {
				confirmOrders();
				return;
			}

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
						m_recorder->recordSystem("Correction failed no deals", SYSTEM_RECORD_TYPE_ERROR);
						fprintf(stderr, "Correction no deals FAILED\n");
						return;
				}
			}
		});

		struct timespec curr{}, prev{};
		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &curr);

			if ((curr.tv_sec - prev.tv_sec) > (0.1 * m_session->heartbtint)) {
				prev = curr;

				if (!lmax_fix_session_keepalive(m_session, &curr)) {
					fprintf(stderr, "TradeOffice: Session keep alive FAILED\n");
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				fprintf(stderr, "TradeOffice: Session time update FAILED\n");
				break;
			}

			if (local_md.size() > 1 && (((curr.tv_sec * 1000000000L) + curr.tv_nsec) -
			                            ((local_md.back().timestamp_ns.tv_sec * 1000000000L) +
			                             local_md.back().timestamp_ns.tv_nsec) > 10000000)) {
				local_md.pop_back();
			}

			if (confirm_delay_check && (((curr.tv_sec * 1000000000L) + curr.tv_nsec) -
			                            ((confirm_delay_start.tv_sec * 1000000000L) + confirm_delay_start.tv_nsec)) >
			                           20000000) {
				confirmOrders();
				confirm_delay_check = false;
			}

			local_md_poller->poll(local_md_handler);
			remote_md_poller->poll(remote_md_handler);
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
