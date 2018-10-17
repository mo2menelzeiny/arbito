
#include "lmax/TradeOffice.h"

namespace LMAX {

	using namespace std::chrono;

	TradeOffice::TradeOffice(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
	                         const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
	                         const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
	                         Recorder &recorder, BrokerConfig broker_config, double lot_size)
			: m_control_buffer(control_buffer), m_business_buffer(business_buffer), m_trade_buffer(trade_buffer),
			  m_recorder(&recorder), m_broker_config(broker_config), m_lot_size(lot_size) {
		lmax_fix_session_cfg_init(&m_cfg);
		m_cfg.dialect = &lmax_fix_dialects[LMAX_FIX_4_4];
		m_cfg.heartbtint = broker_config.heartbeat;
		strncpy(m_cfg.username, broker_config.username, ARRAY_SIZE(m_cfg.username));
		strncpy(m_cfg.password, broker_config.password, ARRAY_SIZE(m_cfg.password));
		strncpy(m_cfg.sender_comp_id, broker_config.sender, ARRAY_SIZE(m_cfg.sender_comp_id));
		strncpy(m_cfg.target_comp_id, broker_config.receiver, ARRAY_SIZE(m_cfg.target_comp_id));
	}

	void TradeOffice::start() {
		while (!connectToBroker()) {
			close(m_session->sockfd);
			SSL_free(m_session->ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			std::this_thread::sleep_for(seconds(RECONNECT_DELAY_SEC));
		};

		auto poller = std::thread(&TradeOffice::poll, this);
		poller.detach();
	}

	bool TradeOffice::connectToBroker() {
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

		m_session = lmax_fix_session_new(&m_cfg);
		if (!m_session) {
			m_recorder->systemEvent("TradeOffice: FIX session FAILED", SE_TYPE_ERROR);
			return false;
		}

		struct hostent *host_ent = gethostbyname(m_broker_config.host);

		if (!host_ent) {
			m_recorder->systemEvent("TradeOffice: Host lookup FAILED", SE_TYPE_ERROR);
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
			std::stringstream ss;
			ss << "TradeOffice: Socket connection FAILED " << saved_errno;
			const std::string& tmp = ss.str();
			const char *cstr = tmp.c_str();
			m_recorder->systemEvent(cstr, SE_TYPE_ERROR);
			return false;
		}

		if (lmax_socket_setopt(m_cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0) {
			m_recorder->systemEvent("TradeOffice: Socket option FAILED", SE_TYPE_ERROR);
			return false;
		}

		SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
		int ssl_errno = SSL_connect(m_cfg.ssl);

		if (ssl_errno <= 0) {
			m_recorder->systemEvent("TradeOffice: SSL FAILED", SE_TYPE_ERROR);
			return false;
		}

		fcntl(m_cfg.sockfd, F_SETFL, O_NONBLOCK);

		if (lmax_fix_session_logon(m_session)) {
			m_recorder->systemEvent("TradeOffice: Logon FAILED", SE_TYPE_ERROR);
			return false;
		}
		m_recorder->systemEvent("TradeOffice: Logon OK", SE_TYPE_SUCCESS);

		return true;
	}

	void TradeOffice::poll() {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(2, &cpuset);
		pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
		pthread_setname_np(pthread_self(), "trade");

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
			return false;
		};

		struct timespec curr{}, prev{};
		clock_gettime(CLOCK_MONOTONIC, &prev);

		while (m_session->active) {
			clock_gettime(CLOCK_MONOTONIC, &curr);

			if ((curr.tv_sec - prev.tv_sec) > (0.1 * m_session->heartbtint)) {
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

			business_poller->poll(business_handler);

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
				case LMAX_FIX_MSG_TYPE_EXECUTION_REPORT:
					if (lmax_fix_get_field(msg, lmax_ExecType)->string_value[0] == 'F') {
						auto data = TradeEvent();
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_OrderID), data.orderId, 64);
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_ClOrdID), data.clOrdId, 64);
						data.side = lmax_fix_get_field(msg, lmax_Side)->string_value[0];
						data.avgPx = lmax_fix_get_field(msg, lmax_AvgPx)->float_value;
						data.timestamp_ms = (curr.tv_sec * 1000) + (curr.tv_nsec / 1000000);

						try {
							auto next_record = m_recorder->m_trade_records_buffer->tryNext();
							(*m_recorder->m_trade_records_buffer)[next_record] = data;
							m_recorder->m_trade_records_buffer->publish(next_record);
						} catch (Disruptor::InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"TradeOffice: Trade records buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}
								
					}

					if (lmax_fix_get_field(msg, lmax_ExecType)->string_value[0] == '8') {
						auto data = TradeEvent();
						strcpy(data.orderId, "FAILED");
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_ClOrdID), data.clOrdId, 64);
						data.side = lmax_fix_get_field(msg, lmax_Side)->string_value[0];
						data.avgPx = 0;
						data.timestamp_ms = (curr.tv_sec * 1000) + (curr.tv_nsec / 1000000);

						try {
							auto next_record = m_recorder->m_trade_records_buffer->tryNext();
							(*m_recorder->m_trade_records_buffer)[next_record] = data;
							m_recorder->m_trade_records_buffer->publish(next_record);
						} catch (Disruptor::InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"TradeOffice: Trade records buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						char text[64];
						lmax_fix_get_string(lmax_fix_get_field(msg, lmax_Text), text, 64);
						std::stringstream ss;
						ss << "TradeOffice: Order rejected Text: " << text;
						const std::string &tmp = ss.str();
						const char *cstr = tmp.c_str();
						m_recorder->systemEvent(cstr, SE_TYPE_ERROR);
					}

					continue;

				default:
					lmax_fix_session_admin(m_session, msg);
					continue;
			}

		}

		m_business_buffer->removeGatingSequence(business_poller->sequence());

		try {
			auto next_pause = m_control_buffer->tryNext();
			(*m_control_buffer)[next_pause] = (ControlEvent) {
					.type = CET_PAUSE,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_pause);
		} catch (Disruptor::InsufficientCapacityException &e) {
			m_recorder->systemEvent("TradeOffice: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}

		m_recorder->systemEvent("TradeOffice: Connection FAILED", SE_TYPE_ERROR);

		do {
			close(m_session->sockfd);
			SSL_free(m_session->ssl);
			ERR_free_strings();
			EVP_cleanup();
			lmax_fix_session_free(m_session);
			std::this_thread::sleep_for(seconds(RECONNECT_DELAY_SEC));
		} while (!connectToBroker());

		auto poller = std::thread(&TradeOffice::poll, this);
		poller.detach();

		try {
			auto next_resume = m_control_buffer->tryNext();
			(*m_control_buffer)[next_resume] = (ControlEvent) {
					.type = CET_RESUME,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_resume);
		} catch (Disruptor::InsufficientCapacityException &e) {
			m_recorder->systemEvent("TradeOffice: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}
	}
}
