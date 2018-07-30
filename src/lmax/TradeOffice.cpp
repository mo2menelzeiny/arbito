
#include "lmax/TradeOffice.h"

namespace LMAX {

	TradeOffice::TradeOffice(const char *m_host, int m_port, const char *username, const char *password,
	                         const char *sender_comp_id, const char *target_comp_id, int heartbeat,
	                         const char *pub_channel, const char *pub_stream_id, const char *sub_channel,
	                         const char *sub_stream_id, double diff_open, double diff_close)
			: m_host(m_host), m_port(m_port),m_aeron_config({}), m_diff_open(diff_open), m_diff_close(diff_close) {
		// Session configurations
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[FIX_4_4];
		m_cfg.heartbtint = heartbeat;
		strncpy(m_cfg.username, username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, target_comp_id, ARRAY_SIZE(m_cfg.target_comp_id));

		// Disruptor // TODO: optimize and test on ring buffer size
		/*m_taskscheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
		m_disruptor = std::make_shared<Disruptor::disruptor<Event>>(
				[]() { return Event(); },
				1024,
				m_taskscheduler,
				Disruptor::ProducerType::Single,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());*/
	}

	void TradeOffice::start() {
		/*m_taskscheduler->start();
		m_disruptor->start();*/
		initBrokerClient();
		initMessengerClient();
	}

	void TradeOffice::onEvent(MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) {
		// TODO: test if the pointer loses it's reference at some point or invalidates the memory access
		last_market_data = &data;

		// TODO: test and optimize variable declaration of sbe as global members
		sbe::MessageHeader msgHeader;
		msgHeader.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, AERON_BUFFER_SIZE)
				.blockLength(sbe::MarketData::sbeBlockLength())
				.templateId(sbe::MarketData::sbeTemplateId())
				.schemaId(sbe::MarketData::sbeSchemaId())
				.version(sbe::MarketData::sbeSchemaVersion());

		sbe::MarketData marketData;
		marketData.wrapForEncode(reinterpret_cast<char *>(m_buffer), msgHeader.encodedLength(), AERON_BUFFER_SIZE)
				.bid(data.bid)
				.bidQty(data.bid_qty)
				.offer(data.offer)
				.offerQty(data.offer_qty);

		aeron::index_t len = msgHeader.encodedLength() + marketData.encodedLength();
		aeron::concurrent::AtomicBuffer srcBuffer(m_buffer, AERON_BUFFER_SIZE);
		srcBuffer.putBytes(0, reinterpret_cast<const uint8_t *>(m_buffer), len);
		std::int64_t result;
		do {
			result = m_publication->offer(srcBuffer, 0, len);
		} while (result < 0L);

	}

	void TradeOffice::initBrokerClient() {
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
		if (lmax_fix_session_logon(m_session)) {
			fprintf(stderr, "Client Logon FAILED\n");
			return;
		}
		fprintf(stdout, "Client Logon OK\n");

		// Polling thread loop
		m_broker_client_poller = std::thread(&TradeOffice::pollBrokerClient, this);
		m_broker_client_poller.detach();

	}

	void TradeOffice::pollBrokerClient() {
		struct timespec cur{}, prev{};
		__time_t diff;

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {

			clock_gettime(CLOCK_MONOTONIC, &cur);

			diff = (cur.tv_sec - prev.tv_sec);

			if (diff > 0.1 * m_session->heartbtint) {
				prev = cur;

				if (!lmax_fix_session_keepalive(m_session, &cur)) {
					fprintf(stderr, "Session keep alive FAILED\n");
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				fprintf(stderr, "Session time update FAILED\n");
				break;
			}

			struct lmax_fix_message *msg = nullptr;
			if (lmax_fix_session_recv(m_session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				if (!msg) {
					continue;
				}

				if (lmax_fix_session_admin(m_session, msg)) {
					continue;
				}
			}
			// TODO: Implement trade messages handling
			fprintmsg(stdout, msg);
			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_HEARTBEAT:
					/*fprintmsg(stdout, msg);*/
					continue;
				default:
					continue;
			}
		}

		// Reconnection condition
		if (m_session->active) {
			std::this_thread::sleep_for(std::chrono::seconds(60));
			fprintf(stdout, "Trade office client reconnecting..\n");
			SSL_shutdown(m_cfg.ssl);
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			initBrokerClient();
		}
	}

	void TradeOffice::messengerMediaDriver() {
		aeron_driver_context_t *context = nullptr;
		aeron_driver_t *driver = nullptr;

		if (aeron_driver_context_init(&context) < 0) {
			fprintf(stderr, "ERROR: context init (%d) %s\n", aeron_errcode(), aeron_errmsg());
			goto cleanup;
		}

		if (aeron_driver_init(&driver, context) < 0) {
			fprintf(stderr, "ERROR: driver init (%d) %s\n", aeron_errcode(), aeron_errmsg());
			goto cleanup;
		}

		if (aeron_driver_start(driver, true) < 0) {
			fprintf(stderr, "ERROR: driver start (%d) %s\n", aeron_errcode(), aeron_errmsg());
			goto cleanup;
		}

		loop:
		aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
		goto loop;

		cleanup:
		aeron_driver_close(driver);
		aeron_driver_context_close(context);
	}

	void TradeOffice::initMessengerClient() {
		// Aeron media driver thread
		m_media_driver = std::thread(&TradeOffice::messengerMediaDriver, this);
		m_media_driver.detach();

		// Aeron configurations
		m_aeron_context.newSubscriptionHandler([](const std::string &channel, std::int32_t streamId,
		                                          std::int64_t correlationId) {
			std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
		});

		m_aeron_context.newPublicationHandler([](const std::string &channel, std::int32_t streamId,
		                                         std::int32_t sessionId, std::int64_t correlationId) {
			std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":"
			          << sessionId << std::endl;
		});

		m_aeron_context.availableImageHandler([&](aeron::Image &image) {
			std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
			          << image.sessionId();
			std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
		});

		m_aeron_context.unavailableImageHandler([](aeron::Image &image) {
			std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
			          << image.sessionId();
			std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
		});

		m_aeron = std::make_shared<aeron::Aeron>(m_aeron_context);

		std::int64_t publication_id = m_aeron->addPublication(m_aeron_config.pub_channel, m_aeron_config.pub_stream_id);
		std::int64_t subscription_id = m_aeron->addSubscription(m_aeron_config.sub_channel,
		                                                        m_aeron_config.sub_stream_id);

		m_publication = m_aeron->findPublication(publication_id);
		while (!m_publication) {
			std::this_thread::yield();
			m_publication = m_aeron->findPublication(publication_id);
		}
		printf("Publication found!\n");

		m_subscription = m_aeron->findSubscription(subscription_id);
		while (!m_subscription) {
			std::this_thread::yield();
			m_subscription = m_aeron->findSubscription(subscription_id);
		}
		printf("Subscription found!\n");

		// Polling thread
		m_messenger_client_poller = std::thread(&TradeOffice::pollMessengerClient, this);
		m_messenger_client_poller.detach();
	}

	void TradeOffice::pollMessengerClient() {
		aeron::BusySpinIdleStrategy idleStrategy;
		aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
		                                               aeron::index_t length, const aeron::Header &header) {
			sbe::MessageHeader msgHeader;
			msgHeader.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, AERON_BUFFER_SIZE);

			// TODO: refactor to only accepts market data messages and create another channel for ACK messages
			sbe::MarketData marketData;
			marketData.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
			                         msgHeader.encodedLength(), msgHeader.blockLength(), msgHeader.version(),
			                         AERON_BUFFER_SIZE);

			onMessengerMarketData(marketData);
		});

		while (true) {
			idleStrategy.idle(m_subscription->poll(fragmentAssembler.handler(), 10));
		}
	}

	void TradeOffice::onMessengerMarketData(sbe::MarketData &marketData) {

		switch (m_market_state) {
			case NO_DEALS:
				// current difference 1 -> offer1 - bid2
				if (last_market_data->offer - marketData.bid() >= m_diff_open) {
					// TODO: open a new order
					return;
				}
					// current difference 2 -> offer2 - bid1
				if (marketData.offer() - last_market_data->bid >= m_diff_open) {
					// TODO: open a new order
					return;
				}
				break;
			case CURRENT_DIFFERENCE_ONE:
				if (last_market_data->offer - marketData.bid() >= m_diff_close) {
					// TODO: close oldest order
				}
				if (deals.size() < MAX_DEALS && last_market_data->offer - marketData.bid() >= m_diff_open) {
					// TODO: open a new order
				}
				break;
			case CURRENT_DIFFERENCE_TWO:
				if (marketData.offer() - last_market_data->bid >= m_diff_close) {
					// TODO: close oldest order
				}
				if (deals.size() < MAX_DEALS && marketData.offer() - last_market_data->bid >= m_diff_open) {
					// TODO: open new order
				}
				break;
		}

	}
}
