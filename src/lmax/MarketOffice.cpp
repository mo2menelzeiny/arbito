
#include "lmax/MarketOffice.h"

namespace LMAX {

	MarketOffice::MarketOffice(
			const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
			Recorder &recorder, Messenger &messenger, BrokerConfig broker_config, double spread, double lot_size)
			: m_control_buffer(control_buffer), m_local_md_buffer(local_md_buffer), m_recorder(&recorder),
			  m_messenger(&messenger), m_broker_config(broker_config), m_spread(spread), m_lot_size(lot_size) {
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));
		m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
	}

	void MarketOffice::start() {
		initBrokerClient();
	}

	void MarketOffice::initBrokerClient() {
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
			m_recorder->recordSystem("MarketOffice: FIX Session FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: FIX session cannot be created\n");
			return;
		}

		// Socket connection
		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent) {
			m_recorder->recordSystem("MarketOffice: Host lookup FAILED", SYSTEM_RECORD_TYPE_ERROR);
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
			m_recorder->recordSystem("MarketOffice: Socket connection FAILED", SYSTEM_RECORD_TYPE_ERROR);
			error("MarketOffice: Unable to connect to a socket (%s)", strerror(saved_errno));
		}

		if (lmax_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0) {
			m_recorder->recordSystem("MarketOffice: Socket option FAILED", SYSTEM_RECORD_TYPE_ERROR);
			die("MarketOffice: cannot set socket option TCP_NODELAY");
		}

		// SSL connection
		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			fprintf(stderr, "MarketOffice: SSL FAILED\n");
			m_recorder->recordSystem("MarketOffice: SSL FAILED", SYSTEM_RECORD_TYPE_ERROR);
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

		if (lmax_fix_session_logon(m_session)) {
			m_recorder->recordSystem("MarketOffice: broker client logon FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: Client Logon FAILED\n");
		}

		if (lmax_fix_session_marketdata_request(m_session)) {
			m_recorder->recordSystem("MarketOffice: market data request FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: Client market data request FAILED\n");
		}

		poller = std::thread(&MarketOffice::poll, this);
		poller.detach();
	}

	void MarketOffice::poll() {
		struct timespec curr{}, prev{};
		sbe::MessageHeader sbe_header;
		sbe::MarketData sbe_market_data;
		bool pause = false;
		auto control_poller = m_control_buffer->newPoller();
		m_control_buffer->addGatingSequences({control_poller->sequence()});
		auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
			if (data.source == CES_MARKET_OFFICE) {
				return true;
			}

			switch (data.type) {
				case CET_PAUSE: {
					pause = true;
					sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
							.blockLength(sbe::MarketData::sbeBlockLength())
							.templateId(sbe::MarketData::sbeTemplateId())
							.schemaId(sbe::MarketData::sbeSchemaId())
							.version(sbe::MarketData::sbeSchemaVersion());
					sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
					                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
							.bid(-99)
							.offer(99)
							.timestamp((curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L));
					aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();
					std::int64_t result;

					do {
						result = m_messenger->marketDataPub()->offer(m_atomic_buffer, 0, len);
					} while (result < -1);

					break;
				}

				case CET_RESUME:
					pause = false;
					break;

				default:
					break;
			}

			return true;
		};

		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			clock_gettime(CLOCK_MONOTONIC, &curr);

			if ((curr.tv_sec - prev.tv_sec) > 0.1 * m_session->heartbtint) {
				prev = curr;

				if (!lmax_fix_session_keepalive(m_session, &curr)) {
					break;
				}
			}

			if (lmax_fix_session_time_update(m_session)) {
				break;
			}

			control_poller->poll(control_handler);

			struct lmax_fix_message *msg;
			if (lmax_fix_session_recv(m_session, &msg, LMAX_FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
				continue;
			}

			switch (msg->type) {
				case LMAX_FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
					if (pause) {
						continue;
					}

					if (m_spread < (lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value -
					                lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0))
					    || m_lot_size > lmax_fix_get_float(msg, lmax_MDEntrySize, 0.0)
					    || m_lot_size > lmax_fix_get_field_at(msg, msg->nr_fields - 1)->float_value) {
						continue;
					}

					sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
							.blockLength(sbe::MarketData::sbeBlockLength())
							.templateId(sbe::MarketData::sbeTemplateId())
							.schemaId(sbe::MarketData::sbeSchemaId())
							.version(sbe::MarketData::sbeSchemaVersion());
					sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
					                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
							.bid(lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0))
							.offer(lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value)
							.timestamp((curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L));
					aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();
					std::int64_t result;
					do {
						result = m_messenger->marketDataPub()->offer(m_atomic_buffer, 0, len);
					} while (result < -1);

					auto next = m_local_md_buffer->next();
					(*m_local_md_buffer)[next] = (MarketDataEvent) {
							.bid = lmax_fix_get_float(msg, lmax_MDEntryPx, 0.0),
							.offer = lmax_fix_get_field_at(msg, msg->nr_fields - 2)->float_value,
							.timestamp_us  = (curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L)
					};
					m_local_md_buffer->publish(next);
					continue;
				}
				case LMAX_FIX_MSG_TYPE_TEST_REQUEST:
					lmax_fix_session_admin(m_session, msg);
					continue;
				default:
					continue;
			}
		}

		if (m_session->active) {
			auto next_pause = m_control_buffer->next();
			(*m_control_buffer)[next_pause] = (ControlEvent) {
					.source = CES_MARKET_OFFICE,
					.type = CET_PAUSE,
					.timestamp_us  = (curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L)
			};
			m_control_buffer->publish(next_pause);

			m_recorder->recordSystem("MarketOffice: Session FAILED", SYSTEM_RECORD_TYPE_ERROR);
			fprintf(stderr, "MarketOffice: Session FAILED\n");
			std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));
			SSL_free(m_cfg.ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			initBrokerClient();

			auto next_resume = m_control_buffer->next();
			(*m_control_buffer)[next_resume] = (ControlEvent) {
					.source = CES_MARKET_OFFICE,
					.type = CET_RESUME,
					.timestamp_us  = (curr.tv_sec * 1000000L) + (curr.tv_nsec / 1000L)
			};
			m_control_buffer->publish(next_resume);
		}

	}
}