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


/*#include <Disruptor/Disruptor.h>
#include <Disruptor/ThreadPerTaskScheduler.h>
#include <Disruptor/BasicExecutor.h>
#include <Disruptor/BusySpinWaitStrategy.h>
#include <Disruptor/RingBuffer.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ILifecycleAware.h>
#include <Disruptor/AggregateEventHandler.h>*/

#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <Configuration.h>
#include <FragmentAssembler.h>
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>

extern "C" {


/*
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
    const SSL_METHOD *meth;

    InitializeSSL();

    meth = TLS_client_method();
    ssl_ctx = SSL_CTX_new(meth);
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
*/

volatile bool running_md = true;

inline bool aeron_is_running() {
    bool result;
    AERON_GET_VOLATILE(result, running_md);
    return result;
}


int aeron_driver() {

    int status = EXIT_FAILURE;
    aeron_driver_context_t *context = NULL;
    aeron_driver_t *driver = NULL;

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

    while (aeron_is_running()) {
        aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
    }

    printf("Shutting down driver...\n");

    cleanup:

    aeron_driver_close(driver);
    aeron_driver_context_close(context);

    return status;
}


}

/*struct TestEvent {
    long value;
};*/

/*
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
*/

using namespace aeron;
using namespace std::chrono;

std::atomic<bool> running(true);

int sub() {

    try {
        float sum = 0;
        long count = 0;

        std::cout << "Subscribing to channel " << getenv("SUB_CHANNEL") << " on Stream ID " << getenv("SUB_STREAM_ID")
                  << std::endl;

        aeron::Context context;

        context.newSubscriptionHandler(
                [](const std::string &channel, std::int32_t streamId, std::int64_t correlationId) {
                    std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
                });

        context.availableImageHandler([](Image &image) {
            std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        context.unavailableImageHandler([](Image &image) {
            std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);

        // add the subscription to start the process
        std::int64_t id = aeron->addSubscription(getenv("SUB_CHANNEL"), atoi(getenv("SUB_STREAM_ID")));

        std::shared_ptr<Subscription> subscription = aeron->findSubscription(id);
        // wait for the subscription to be valid
        while (!subscription) {
            std::this_thread::yield();
            subscription = aeron->findSubscription(id);
        }

        fragment_handler_t handler = [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length,
                                         const Header &header) {
            steady_clock::time_point rec_time = steady_clock::now();
            steady_clock::time_point send_time;
            buffer.getBytes(offset, (uint8_t *) &send_time, sizeof(steady_clock::time_point));
            ++count;
            sum += duration_cast<nanoseconds>(rec_time - send_time).count();

            if (count > 1000) {
                const float result = sum / count / 1000000;
                std::cout << std::fixed;
                std::cout << std::setprecision(6);
                std::cout << "average duration: " << result << " ms/msg " << "Batch: " << count << "\n";
                count = 0;
                sum = 0;
            }
        };

        BusySpinIdleStrategy idleStrategy;

        while (running) {
            idleStrategy.idle(subscription->poll(handler, 10));
        }

    }
    catch (const SourcedException &e) {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }

    return 0;
}

int pub() {
    try {
        std::cout << "Publishing to channel " << getenv("PUB_CHANNEL") << " on Stream ID " << getenv("PUB_STREAM_ID")
                  << std::endl;

        aeron::Context context;

        context.newPublicationHandler(
                [](const std::string &channel, std::int32_t streamId, std::int32_t sessionId,
                   std::int64_t correlationId) {
                    std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":"
                              << sessionId << std::endl;
                });

        context.availableImageHandler([](Image &image) {
            std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        context.unavailableImageHandler([](Image &image) {
            std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
                      << image.sessionId();
            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
        });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);

        // add the publication to start the process
        std::int64_t id = aeron->addPublication(getenv("PUB_CHANNEL"), atoi(getenv("PUB_STREAM_ID")));

        std::shared_ptr<Publication> publication = aeron->findPublication(id);
        // wait for the publication to be valid
        while (!publication) {
            std::this_thread::yield();
            publication = aeron->findPublication(id);
        }

        std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[256]);
        concurrent::AtomicBuffer srcBuffer(buffer.get(), 256);

        do {
            std::chrono::steady_clock::time_point send_time = std::chrono::steady_clock::now();
            srcBuffer.putBytes(0, (uint8_t *) &send_time, sizeof(std::chrono::steady_clock::time_point));
            const std::int64_t result = publication->offer(srcBuffer, 0, sizeof(srcBuffer));

            if (result < 0) {
                if (BACK_PRESSURED == result) {
                    std::cout << "Offer failed due to back pressure" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else if (NOT_CONNECTED == result) {
                    std::cout << "Offer failed because publisher is not connected to subscriber" << std::endl;
                } else if (ADMIN_ACTION == result) {
                    std::cout << "Offer failed because of an administration action in the system" << std::endl;
                } else if (PUBLICATION_CLOSED == result) {
                    std::cout << "Offer failed publication is closed" << std::endl;
                } else {
                    std::cout << "Offer failed due to unknown reason" << result << std::endl;
                }
            }

            if (!publication->isConnected()) {
                std::cout << "No active subscribers detected" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(atoi(getenv("RATE_MS"))));
        } while (running);
    }
    catch (const SourcedException &e) {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }
}

int main() {

    try {
        std::thread aeron_md_thread(aeron_driver);
        std::thread subscription;
        std::thread publication;

        switch (atoi(getenv("MODE"))) {
            case 0:
                subscription = std::thread(sub);
                break;
            case 1:
                publication = std::thread(pub);
                break;
            case 2:
                subscription = std::thread(sub);
                publication = std::thread(pub);
                break;
            default:
                std::cout << "Invalid Mode\n";
        }

        while (running) {
            std::this_thread::yield();
        }
    }
    catch (const SourcedException &e) {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception &e) {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}