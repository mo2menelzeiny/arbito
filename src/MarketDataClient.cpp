
#include "MarketDataClient.h"

namespace LMAX {

    MarketDataClient::MarketDataClient(const char *m_host, int m_port, const char *username, const char *password,
                                       const char *sender_comp_id, const char *target_comp_id, int heartbeat) : m_host(m_host),
                                                                                                 m_port(m_port) {
        // session configurations
        lmax_fix_session_cfg_init(&m_cfg);
        m_cfg.dialect = &lmax_fix_dialects[FIX_4_4];
        m_cfg.heartbtint = heartbeat;
        strncpy(m_cfg.username, username, ARRAY_SIZE(m_cfg.username));
        strncpy(m_cfg.password, password, ARRAY_SIZE(m_cfg.password));
        strncpy(m_cfg.sender_comp_id, sender_comp_id, ARRAY_SIZE(m_cfg.sender_comp_id));
        strncpy(m_cfg.target_comp_id, target_comp_id, ARRAY_SIZE(m_cfg.target_comp_id));

        // ssl
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

        // session
        m_session = lmax_fix_session_new(&m_cfg);

        // disruptor
        m_taskscheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
        m_disruptor = std::make_shared<Disruptor::disruptor<DataEvent>>(
                []() { return DataEvent(); },
                1024,
                m_taskscheduler,
                Disruptor::ProducerType::Single,
                std::make_shared<Disruptor::BusySpinWaitStrategy>());

        m_disruptor->handleEventsWith(std::make_shared<PrintEventHandler>());
        m_taskscheduler->start();
        m_disruptor->start();
    }

    void MarketDataClient::start() {

        if (!m_session) {
            fprintf(stderr, "FIX session cannot be created\n");
            this->stop();
        }

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


        SSL_set_fd(m_cfg.ssl, m_cfg.sockfd);
        int ssl_errno = SSL_connect(m_cfg.ssl);

        if (ssl_errno <= 0) {
            fprintf(stderr, "SSL ERROR\n");
            this->stop();
        }

        X509 *server_cert;
        char *str;
        printf("SSL connection using %s\n", SSL_get_cipher (m_ssl));

        server_cert = SSL_get_peer_certificate(m_ssl);
        printf("Server certificate:\n");

        str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
        printf("\t subject: %s\n", str);
        OPENSSL_free(str);

        str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
        printf("\t issuer: %s\n", str);
        OPENSSL_free(str);
    }

    void MarketDataClient::stop() {
        m_taskscheduler->stop();
        m_disruptor->shutdown();
        SSL_shutdown(m_cfg.ssl);
        SSL_free(m_cfg.ssl);
        ERR_free_strings();
        EVP_cleanup();
        lmax_fix_session_free(m_session);
    }

    void MarketDataClient::handle() {
        struct timespec cur{}, prev{};
        int diff, stop = 0;

        if (lmax_fix_session_logon(m_session)) {
            fprintf(stderr, "Client Logon FAILED\n");
            return this->stop();
        }
        fprintf(stdout, "Client Logon OK\n");

        if(lmax_fix_session_marketdata_request(m_session)) {
            fprintf(stderr, "Market data request FAILED\n");
            return this->stop();
        }
        fprintf(stdout, "Market Data Request OK\n");

        clock_gettime(CLOCK_MONOTONIC, &prev);

        while (!stop && m_session->active) {

            clock_gettime(CLOCK_MONOTONIC, &cur);

            diff = static_cast<int>(cur.tv_sec - prev.tv_sec);

            if (diff > 0.1 * m_session->heartbtint) {
                prev = cur;

                if (!lmax_fix_session_keepalive(m_session, &cur)) {
                    fprintf(stderr, "session keep alive FAILED\n");
                    stop = 1;
                    break;
                }
            }

            if (lmax_fix_session_time_update(m_session)) {
                fprintf(stderr, "Session time update FAILED\n");
                stop = 1;
                break;
            }

            struct lmax_fix_message *msg = nullptr;
            if (lmax_fix_session_recv(m_session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
                if(!msg)
	                continue;

                if (lmax_fix_session_admin(m_session, msg))
                    continue;

                switch (msg->type) {
                    case LMAX_FIX_MSG_TYPE_LOGOUT:
                        stop = 1;
                        if (lmax_fix_session_logout(m_session, nullptr)) {
                            fprintf(stderr, "Client Logout FAILED\n");
                            return this->stop();
                        }
                        fprintf(stdout, "Client Logout OK\n");
                        break;
                    default:
                        stop = 1;
                        break;
                }
            } else {
                fprintf(stdout, "normal:");
                fprintmsg(stdout, msg);
            }
        }

        if (m_session->active) {
            if (lmax_fix_session_logout(m_session, nullptr)) {
                fprintf(stderr, "Client Logout FAILED\n");
                return this->stop();
            } else {
                fprintf(stdout, "Client Logout OK\n");
                this->stop();
            }
        }

        printf("Client Terminated\n");
        this->stop();
    }

    void MarketDataClient::onData(lmax_fix_message *msg) {
        auto next_sequence = m_disruptor->ringBuffer()->next();
        lmax_fix_field *field = lmax_fix_get_field(msg, SendingTime);
        (*m_disruptor->ringBuffer())[next_sequence].value = lmax_fix_get_string(field, m_buffer,
                                                                                strlen(field->string_value));
        m_disruptor->ringBuffer()->publish(next_sequence);
    }

    void PrintEventHandler::onEvent(DataEvent &data, std::int64_t sequence, bool endOfBatch) {
        printf("Value: %s \n", data.value);
    }
}