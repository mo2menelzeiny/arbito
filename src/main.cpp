#include <iostream>
#include <chrono>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <inttypes.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <float.h>
#include <netdb.h>
#include <stdio.h>
#include <math.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <libtrading/proto/lmax_common.h>
#include <libtrading/die.h>
#include <libtrading/array.h>

#include <memory>
#include <Disruptor/Disruptor.h>
#include <Disruptor/ThreadPerTaskScheduler.h>
#include <Disruptor/BasicExecutor.h>
#include <Disruptor/BusySpinWaitStrategy.h>
#include <Disruptor/RingBuffer.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ILifecycleAware.h>
#include <Disruptor/AggregateEventHandler.h>

#include <Aeron.h>

void benchmark() {
    std::string str("8=FIX.4.4 9=75 35=A 49=LMXBD 56=AhmedDEMO 34=1 52=20180531-04:52:25.328 98=0 108=30 141=Y 10=173");
    long count = 0, sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }
    std::cout << "wasted: " << sum / count << std::endl;
    count = 0;
    sum = 0;
    while (count <= 1000000) {
        count++;
        auto t1 = std::chrono::steady_clock::now();
        // put test case here
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }
    std::cout << "total: " << sum / count << std::endl;
}

extern "C" {

void InitializeSSL() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL() {
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL(SSL *cSSL) {
    SSL_shutdown(cSSL);
    SSL_free(cSSL);
}

static int socket_setopt(int sockfd, int level, int optname, int optval) {
    return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
}

static int fstrncpy(char *dest, const char *src, int n) {
    int i;

    /* If n is negative nothing is copied */
    for (i = 0; i < n && src[i] != 0x00 && src[i] != 0x01; i++)
        dest[i] = src[i];

    return i;
}

static void fprintmsg(FILE *stream, struct lmax_fix_message *msg) {
    char buf[265];
    struct lmax_fix_field *field;
    int size = sizeof buf;
    char delim = '|';
    int len = 0;
    int i;

    if (!msg)
        return;

    if (msg->begin_string && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=", delim, BeginString);
        len += fstrncpy(buf + len, msg->begin_string, size - len);
    }

    if (msg->body_length && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=%lu", delim, BodyLength, msg->body_length);
    }

    if (msg->msg_type && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=", delim, MsgType);
        len += fstrncpy(buf + len, msg->msg_type, size - len);
    }

    if (msg->sender_comp_id && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=", delim, SenderCompID);
        len += fstrncpy(buf + len, msg->sender_comp_id, size - len);
    }

    if (msg->target_comp_id && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=", delim, TargetCompID);
        len += fstrncpy(buf + len, msg->target_comp_id, size - len);
    }

    if (msg->msg_seq_num && len < size) {
        len += snprintf(buf + len, size - len, "%c%d=%lu", delim, MsgSeqNum, msg->msg_seq_num);
    }

    for (i = 0; i < msg->nr_fields && len < size; i++) {
        field = msg->fields + i;

        switch (field->type) {
            case FIX_TYPE_STRING:
                len += snprintf(buf + len, size - len, "%c%d=", delim, field->tag);
                len += fstrncpy(buf + len, field->string_value, size - len);
                break;
            case FIX_TYPE_FLOAT:
                len += snprintf(buf + len, size - len, "%c%d=%f", delim, field->tag, field->float_value);
                break;
            case FIX_TYPE_CHAR:
                len += snprintf(buf + len, size - len, "%c%d=%c", delim, field->tag, field->char_value);
                break;
            case FIX_TYPE_CHECKSUM:
            case FIX_TYPE_INT:
                len += snprintf(buf + len, size - len, "%c%d=%" PRId64, delim, field->tag, field->int_value);
                break;
            default:
                break;
        }
    }

    if (len < size)
        buf[len++] = '\0';

    fprintf(stream, "%s%c\n", buf, delim);
}

int lmax_md_connect() {
    const char *host = "fix-marketdata.london-demo.lmax.com";
    struct sockaddr_in sa;
    int saved_errno = 0;
    struct hostent *he;
    int port = 443;
    int ret = -1;
    int diff;
    int stop = 0;
    char **ap;
    struct lmax_fix_session_cfg cfg;
    struct lmax_fix_session *session = nullptr;
    struct timespec cur, prev;
    struct lmax_fix_message *msg;
    int ssl_err;
    SSL_CTX *ssl_ctx;
    SSL *ssl;

    InitializeSSL();
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_DH_USE);
    int use_cert = SSL_CTX_use_certificate_file(ssl_ctx, "./resources/cert.pem", SSL_FILETYPE_PEM);
    int use_prv = SSL_CTX_use_PrivateKey_file(ssl_ctx, "./resources/key.pem", SSL_FILETYPE_PEM);


    lmax_fix_session_cfg_init(&cfg);

    cfg.dialect = &lmax_fix_dialects[FIX_4_4];
    cfg.heartbtint = 15;
    strncpy(cfg.username, "AhmedDEMO", ARRAY_SIZE(cfg.username));
    strncpy(cfg.password, "password1", ARRAY_SIZE(cfg.password));
    strncpy(cfg.sender_comp_id, "AhmedDEMO", ARRAY_SIZE(cfg.sender_comp_id));
    strncpy(cfg.target_comp_id, "LMXBDM", ARRAY_SIZE(cfg.sender_comp_id));

    ssl = SSL_new(ssl_ctx);
    cfg.ssl = ssl;

    session = lmax_fix_session_new(&cfg);
    if (!session) {
        fprintf(stderr, "FIX session cannot be created\n");
        goto exit;
    }

    he = gethostbyname(host);
    if (!he)
        error("Unable to look up %s (%s)", host, hstrerror(h_errno));

    for (ap = he->h_addr_list; *ap; ap++) {

        cfg.sockfd = socket(he->h_addrtype, SOCK_STREAM, IPPROTO_TCP);
        if (cfg.sockfd < 0) {
            saved_errno = errno;
            continue;
        }

        sa = (struct sockaddr_in) {
                .sin_family        = static_cast<sa_family_t>(he->h_addrtype),
                .sin_port        = htons(static_cast<uint16_t>(port)),
        };
        memcpy(&sa.sin_addr, *ap, static_cast<size_t>(he->h_length));

        if (connect(cfg.sockfd, (const struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
            saved_errno = errno;
            close(cfg.sockfd);
            cfg.sockfd = -1;
            continue;
        }
        break;
    }

    if (cfg.sockfd < 0)
        error("Unable to connect to a socket (%s)", strerror(saved_errno));

    if (socket_setopt(cfg.sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
        die("cannot set socket option TCP_NODELAY");


    SSL_set_fd(cfg.ssl, cfg.sockfd);
    ssl_err = SSL_connect(cfg.ssl);

    if (ssl_err <= 0) {
        fprintf(stderr, "SSL ERROR\n");
        ShutdownSSL(cfg.ssl);
        goto exit;
    }

    X509 *server_cert;
    char *str;
    printf("SSL connection using %s\n", SSL_get_cipher (ssl));
    server_cert = SSL_get_peer_certificate(ssl);
    printf("Server certificate:\n");
    str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
    printf("\t subject: %s\n", str);
    OPENSSL_free (str);

    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
    printf("\t issuer: %s\n", str);
    OPENSSL_free (str);

    ret = lmax_fix_session_logon(session);
    if (ret) {
        fprintf(stderr, "Client Logon FAILED\n");
        goto exit;
    }
    fprintf(stdout, "Client Logon OK\n");


    clock_gettime(CLOCK_MONOTONIC, &prev);

    while (!stop && session->active) {
        clock_gettime(CLOCK_MONOTONIC, &cur);
        diff = cur.tv_sec - prev.tv_sec;

        if (diff > 0.1 * session->heartbtint) {
            prev = cur;

            if (!lmax_fix_session_keepalive(session, &cur)) {
                stop = 1;
                break;
            }
        }

        if (lmax_fix_session_time_update(session)) {
            stop = 1;
            break;
        }

        if (lmax_fix_session_recv(session, &msg, FIX_RECV_FLAG_MSG_DONTWAIT) <= 0) {
            fprintmsg(stdout, msg);

            if (lmax_fix_session_admin(session, msg))
                continue;

            switch (msg->type) {
                case LMAX_FIX_MSG_TYPE_LOGOUT:
                    stop = 1;
                    break;
                default:
                    stop = 1;
                    break;
            }
        }
    }

    if (session->active) {
        ret = lmax_fix_session_logout(session, NULL);
        if (ret) {
            fprintf(stderr, "Client Logout FAILED\n");
            goto exit;
        }
    }

    fprintf(stdout, "Client Logout OK\n");

    exit:
    lmax_fix_session_free(session);

    return ret;
}

}

struct TestEvent {
    long value;
};

struct EngineEventHandler : Disruptor::IEventHandler<TestEvent> {
    void onEvent(TestEvent &event, int64_t, bool) override {
        std::cout << "Engine: " << event.value << "\n";
    }
};

struct AeronEventHandler : Disruptor::IEventHandler<TestEvent>, Disruptor::ILifecycleAware {
    void onEvent(TestEvent &event, int64_t, bool) override {
        std::cout << "Aeron: " << event.value << "\n";
    }

    void onStart() override {
        m_aeron = aeron::Aeron::connect(m_context);
    }

    void onShutdown() override {

    }

private:
    aeron::Context m_context;
    std::shared_ptr<aeron::Aeron> m_aeron;
};

int main() {
    auto taskScheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
    auto strategy = std::make_shared<Disruptor::BusySpinWaitStrategy>();
    auto testEventFactory = []() {
        return TestEvent();
    };
    auto disruptor = std::make_shared<Disruptor::disruptor<TestEvent>>(testEventFactory, 1024, taskScheduler,
                                                                       Disruptor::ProducerType::Single,
                                                                       strategy);
    auto engineEventHandler = std::make_shared<EngineEventHandler>();
    auto aeronEventHandler = std::make_shared<AeronEventHandler>();

    disruptor->handleEventsWith(aeronEventHandler)->handleEventsWith(engineEventHandler);
    taskScheduler->start();
    disruptor->start();

    auto ringBuffer = disruptor->ringBuffer();
    auto nextSeq = disruptor->ringBuffer()->next();
    (*ringBuffer)[nextSeq].value = 1;
    ringBuffer->publish(nextSeq);

    std::cout << std::this_thread::get_id() << "\n";

    while (true) {
        usleep(100);
    }

    //lmax_md_connect();
    return EXIT_SUCCESS;
}