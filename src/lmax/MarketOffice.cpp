
#include "lmax/MarketOffice.h"

namespace LMAX {

	MarketOffice::MarketOffice(const std::shared_ptr<Recorder> &recorder, const std::shared_ptr<Messenger> &messenger,
	                           const std::shared_ptr<Disruptor::disruptor<MarketDataEvent>> &broker_market_data_disruptor,
	                           const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> &arbitrage_data_disruptor,
	                           const char *m_host, int m_port, const char *username, const char *password,
	                           const char *sender_comp_id,
	                           const char *target_comp_id, int heartbeat, const char *pub_channel, int pub_stream_id,
	                           const char *sub_channel, int sub_stream_id, double spread, double bid_lot_size,
	                           double offer_lot_size)
			: m_messenger(messenger), m_broker_market_data_disruptor(broker_market_data_disruptor),
			  m_arbitrage_data_disruptor(arbitrage_data_disruptor), m_host(m_host), m_port(m_port), m_spread(spread),
			  m_bid_lot_size(bid_lot_size), m_offer_lot_size(offer_lot_size),
			  m_messenger_config{pub_channel, pub_stream_id, sub_channel, sub_stream_id}, m_recorder(recorder) {
		// Session configurations
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = heartbeat;
		strncpy(m_cfg.username, username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, target_comp_id, ARRAY_SIZE(m_cfg.target_comp_id));
	}

	void MarketOffice::start() {
		initMessengerChannel();
		initBrokerClient();
		// broker market data disruptor handler
		m_broker_market_data_handler = std::make_shared<BrokerMarketDataHandler>(m_messenger_pub);
		m_broker_market_data_disruptor->handleEventsWith(m_broker_market_data_handler);
	}

	void MarketOffice::initMessengerChannel() {

		std::int64_t publication_id = m_messenger->aeronClient()->addPublication(m_messenger_config.pub_channel,
		                                                                         m_messenger_config.pub_stream_id);
		std::int64_t subscription_id = m_messenger->aeronClient()->addSubscription(m_messenger_config.sub_channel,
		                                                                           m_messenger_config.sub_stream_id);

		m_messenger_pub = m_messenger->aeronClient()->findPublication(publication_id);
		while (!m_messenger_pub) {
			m_messenger_pub = m_messenger->aeronClient()->findPublication(publication_id);
			std::this_thread::yield();
		}
		printf("MarketOffice: publication found!\n");

		m_messenger_sub = m_messenger->aeronClient()->findSubscription(subscription_id);
		while (!m_messenger_sub) {
			m_messenger_sub = m_messenger->aeronClient()->findSubscription(subscription_id);
			std::this_thread::yield();
		}
		printf("MarketOffice: subscription found!\n");

		m_recorder->recordSystem("MarketOffice: messenger channel OK", SYSTEM_RECORD_TYPE_SUCCESS);
	}

	void MarketOffice::initBrokerClient() {
		printf("MarketOffice: Initializing broker client..\n");
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
			fprintf(stderr, "MarketOffice: Client Logon FAILED\n");
			m_recorder->recordSystem("MarketOffice: broker client logon FAILED", SYSTEM_RECORD_TYPE_ERROR);
			return;
		}
		fprintf(stdout, "MarketOffice: Client Logon OK\n");
		m_recorder->recordSystem("MarketOffice: broker client logon OK", SYSTEM_RECORD_TYPE_SUCCESS);

		// Market data request
		if (lmax_fix_session_marketdata_request(m_session)) {
			fprintf(stderr, "MarketOffice: Client market data request FAILED\n");
			m_recorder->recordSystem("MarketOffice: market data request FAILED", SYSTEM_RECORD_TYPE_ERROR);
			return;
		}
		fprintf(stdout, "MarketOffice: Client market data request OK\n");
		m_recorder->recordSystem("MarketOffice: market data request OK", SYSTEM_RECORD_TYPE_SUCCESS);

		// Polling thread loop
		poller = std::thread(&MarketOffice::poll, this);
		poller.detach();

		m_recorder->recordSystem("MarketOffice: broker client OK", SYSTEM_RECORD_TYPE_SUCCESS);
	}

	void MarketOffice::poll() {
		MarketDataEvent messenger_market_data{.bid = -1.0, .bid_qty = -1.0, .offer = -1.0, .offer_qty = -1.0};
		MarketDataEvent broker_market_data{.bid = -1.0, .bid_qty = -1.0, .offer = -1.0, .offer_qty = -1.0};
		aeron::BusySpinIdleStrategy messengerIdleStrategy;
		sbe::MessageHeader msgHeader;
		sbe::MarketData marketData;
		aeron::FragmentAssembler messengerAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
		                                                aeron::index_t length, const aeron::Header &header) {
			// TODO: implement on the fly decode
			// decode header
			msgHeader.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSEGNER_BUFFER);
			// decode body
			marketData.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
			                         msgHeader.encodedLength(), msgHeader.blockLength(), msgHeader.version(),
			                         MESSEGNER_BUFFER);
			// allocate market data
			messenger_market_data = (MarketDataEvent) {
					.bid = marketData.bid(),
					.bid_qty = marketData.bidQty(),
					.offer = marketData.offer(),
					.offer_qty = marketData.offerQty(),
					.timestamp = marketData.timestamp()
			};

			// publish arbitrage data to arbitrage data disruptor
			auto next_sequence = m_arbitrage_data_disruptor->ringBuffer()->next();
			(*m_arbitrage_data_disruptor->ringBuffer())[next_sequence] = (ArbitrageDataEvent) {
					.l1 = broker_market_data,
					.l2 = messenger_market_data,
					.timestamp = m_session->str_now
			};
			m_arbitrage_data_disruptor->ringBuffer()->publish(next_sequence);

		});

		struct timespec cur{}, prev{};
		__time_t diff;

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &cur);

			diff = (cur.tv_sec - prev.tv_sec);

			if (diff > 0.1 * m_session->heartbtint) {
				prev = cur;

				if (!lmax_fix_session_keepalive(m_session, &cur)) {
					fprintf(stderr, "MarketOffice: Session keep alive FAILED\n");
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				fprintf(stderr, "MarketOffice: Session time update FAILED\n");
				break;
			}

			// messenger subscription poller
			messengerIdleStrategy.idle(m_messenger_sub->poll(messengerAssembler.handler(), 10));

			struct lmax_fix_message *msg = nullptr;
			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				continue;
			}

			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
					// Filter market data based on spread, bid lot size and offer lot size
					if (m_spread > (lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value -
					                lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0))
					    || m_bid_lot_size > lmax_fix_get_float(msg, lmax_MDEntrySize, 0.0)
					    || m_offer_lot_size > lmax_fix_get_field_at(msg, msg->nr_fields - 1)->float_value) {
						continue;
					}

					// allocate timestamp
					char timestamp_buffer[64];
					lmax_fix_get_string(lmax_fix_get_field(msg, lmax_SendingTime),
					                          const_cast<char *>(timestamp_buffer), 64);

					// allocate most recent prices
					broker_market_data = (MarketDataEvent) {
							.bid = lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0),
							.bid_qty = (lmax_fix_get_float(msg, lmax_MDEntrySize, 0.0)),
							.offer = lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value,
							.offer_qty = lmax_fix_get_field_at(msg, msg->nr_fields - 1)->float_value,
							.timestamp = timestamp_buffer
					};

					// publish market data to broker disruptor
					auto next_sequence_1 = m_broker_market_data_disruptor->ringBuffer()->next();
					(*m_broker_market_data_disruptor->ringBuffer())[next_sequence_1] = broker_market_data;
					m_broker_market_data_disruptor->ringBuffer()->publish(next_sequence_1);

					// publish arbitrage data to arbitrage data disruptor
					auto next_sequence_2 = m_arbitrage_data_disruptor->ringBuffer()->next();
					(*m_arbitrage_data_disruptor->ringBuffer())[next_sequence_2] = (ArbitrageDataEvent) {
							.l1 = broker_market_data,
							.l2 = messenger_market_data,
							.timestamp = m_session->str_now
					};
					m_arbitrage_data_disruptor->ringBuffer()->publish(next_sequence_2);

					continue;
				}
				default:
					continue;
			}
		}

		// Reconnection condition
		if (m_session->active) {
			fprintf(stdout, "Market office reconnecting..\n");
			m_recorder->recordSystem("MarketOffice: broker client FAILED", SYSTEM_RECORD_TYPE_ERROR);
			std::this_thread::sleep_for(std::chrono::seconds(30));
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			initBrokerClient();
		}

	}
}