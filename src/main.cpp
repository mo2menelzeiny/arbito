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


#include <Disruptor/Disruptor.h>
#include <Disruptor/ThreadPerTaskScheduler.h>
#include <Disruptor/BasicExecutor.h>
#include <Disruptor/BusySpinWaitStrategy.h>
#include <Disruptor/RingBuffer.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ILifecycleAware.h>
#include <Disruptor/AggregateEventHandler.h>

#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <Configuration.h>
#include <FragmentAssembler.h>


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

using namespace aeron;
using namespace std::chrono;

struct Settings
{
    std::string dirPrefix = "";
    std::string pingChannel = samples::configuration::DEFAULT_PING_CHANNEL;
    std::string pongChannel = samples::configuration::DEFAULT_PONG_CHANNEL;
    std::int32_t pingStreamId = samples::configuration::DEFAULT_PING_STREAM_ID;
    std::int32_t pongStreamId = samples::configuration::DEFAULT_PONG_STREAM_ID;
    long numberOfMessages = samples::configuration::DEFAULT_NUMBER_OF_MESSAGES;
    int messageLength = samples::configuration::DEFAULT_MESSAGE_LENGTH;
    int fragmentCountLimit = samples::configuration::DEFAULT_FRAGMENT_COUNT_LIMIT;
    long numberOfWarmupMessages = samples::configuration::DEFAULT_NUMBER_OF_MESSAGES;
};

void sendPingAndReceivePong(
        const fragment_handler_t& fragmentHandler,
        Publication& publication,
        Subscription& subscription,
        const Settings& settings)
{
    std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[settings.messageLength]);
    concurrent::AtomicBuffer srcBuffer(buffer.get(), settings.messageLength);
    BusySpinIdleStrategy idleStrategy;

    while (0 == subscription.imageCount())
    {
        std::this_thread::yield();
    }

    Image& image = subscription.imageAtIndex(0);

    for (int i = 0; i < settings.numberOfMessages; i++)
    {
        std::int64_t position;

        do
        {
            // timestamps in the message are relative to this app, so just send the timepoint directly.
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            srcBuffer.putBytes(0, (std::uint8_t*)&start, sizeof(std::chrono::steady_clock::time_point));
        }
        while ((position = publication.offer(srcBuffer, 0, settings.messageLength)) < 0L);

        do
        {
            while (image.poll(fragmentHandler, settings.fragmentCountLimit) <= 0)
            {
                idleStrategy.idle();
            }
        }
        while (image.position() < position);
    }
}

std::shared_ptr<Subscription> findSubscription(std::shared_ptr<Aeron> aeron, std::int64_t id)
{
    std::shared_ptr<Subscription> subscription = aeron->findSubscription(id);

    while (!subscription)
    {
        std::this_thread::yield();
        subscription = aeron->findSubscription(id);
    }

    return subscription;
}

std::shared_ptr<Publication> findPublication(std::shared_ptr<Aeron> aeron, std::int64_t id)
{
    std::shared_ptr<Publication> publication = aeron->findPublication(id);

    while (!publication)
    {
        std::this_thread::yield();
        publication = aeron->findPublication(id);
    }

    return publication;
}

std::atomic<bool> running (true);

int main() {

    try
    {
        Settings settings;

        std::cout << "Pong at " << settings.pongChannel << " on Stream ID " << settings.pongStreamId << std::endl;
        std::cout << "Ping at " << settings.pingChannel << " on Stream ID " << settings.pingStreamId << std::endl;

        aeron::Context context;
        std::atomic<int> countDown(1);
        std::int64_t pongSubscriptionId, pingPublicationId, pingSubscriptionId, pongPublicationId;

        context.newSubscriptionHandler(
                [](const std::string& channel, std::int32_t streamId, std::int64_t correlationId)
                {
                    std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
                });

        context.newPublicationHandler(
                [](const std::string& channel, std::int32_t streamId, std::int32_t sessionId, std::int64_t correlationId)
                {
                    std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":" << sessionId << std::endl;
                });

        context.availableImageHandler(
                [&](Image &image)
                {
                    std::cout << "Available image correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
                    std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;

                    if (image.subscriptionRegistrationId() == pongSubscriptionId)
                    {
                        countDown--;
                    }
                });

        context.unavailableImageHandler([](Image &image)
                                        {
                                            std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
                                            std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
                                        });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);

        pongSubscriptionId = aeron->addSubscription(settings.pongChannel, settings.pongStreamId);
        pingPublicationId = aeron->addPublication(settings.pingChannel, settings.pingStreamId);
        pingSubscriptionId = aeron->addSubscription(settings.pingChannel, settings.pingStreamId);
        pongPublicationId = aeron->addPublication(settings.pongChannel, settings.pongStreamId);

        std::shared_ptr<Subscription> pongSubscription, pingSubscription;
        std::shared_ptr<Publication> pingPublication, pongPublication;

        pongSubscription = findSubscription(aeron, pongSubscriptionId);
        pingSubscription = findSubscription(aeron, pingSubscriptionId);
        pingPublication = findPublication(aeron, pingPublicationId);
        pongPublication = findPublication(aeron, pongPublicationId);

        while (countDown > 0)
        {
            std::this_thread::yield();
        }

        Publication& pongPublicationRef = *pongPublication;
        Subscription& pingSubscriptionRef = *pingSubscription;
        BusySpinIdleStrategy idleStrategy;
        BusySpinIdleStrategy pingHandlerIdleStrategy;
        FragmentAssembler pingFragmentAssembler(
                [&](AtomicBuffer& buffer, index_t offset, index_t length, const Header& header)
                {
                    if (pongPublicationRef.offer(buffer, offset, length) > 0L)
                    {
                        return;
                    }

                    while (pongPublicationRef.offer(buffer, offset, length) < 0L)
                    {
                        pingHandlerIdleStrategy.idle();
                    }
                });

        fragment_handler_t ping_handler = pingFragmentAssembler.handler();

        std::thread pongThread(
                [&]()
                {
                    while (0 == pingSubscriptionRef.imageCount())
                    {
                        std::this_thread::yield();
                    }

                    Image& image = pingSubscriptionRef.imageAtIndex(0);

                    while (running)
                    {
                        idleStrategy.idle(image.poll(ping_handler, settings.fragmentCountLimit));
                    }
                });

        if (settings.numberOfWarmupMessages > 0)
        {
            Settings warmupSettings = settings;
            warmupSettings.numberOfMessages = warmupSettings.numberOfWarmupMessages;

            const steady_clock::time_point start = steady_clock::now();

            std::cout << "Warming up the media driver with "
                      << toStringWithCommas(warmupSettings.numberOfWarmupMessages) << " messages of length "
                      << toStringWithCommas(warmupSettings.messageLength) << std::endl;

            sendPingAndReceivePong(
                    [](AtomicBuffer&, index_t, index_t, Header&){}, *pingPublication, *pongSubscription, warmupSettings);

            std::int64_t nanoDuration = duration<std::int64_t, std::nano>(steady_clock::now() - start).count();

            std::cout << "Warmed up the media driver in " << nanoDuration << " [ns]" << std::endl;
        }

        do
        {
            FragmentAssembler fragmentAssembler(
                    [&](const AtomicBuffer& buffer, index_t offset, index_t length, const Header& header)
                    {
                        std::chrono::steady_clock::time_point end = steady_clock::now();
                        steady_clock::time_point start;

                        buffer.getBytes(offset, (std::uint8_t*)&start, sizeof(steady_clock::time_point));
                        std::int64_t nanoRtt = duration<std::int64_t, std::nano>(end - start).count();

                    });

            std::cout << "Pinging "
                      << toStringWithCommas(settings.numberOfMessages) << " messages of length "
                      << toStringWithCommas(settings.messageLength) << " bytes" << std::endl;

            steady_clock::time_point startRun = steady_clock::now();
            sendPingAndReceivePong(fragmentAssembler.handler(), *pingPublication, *pongSubscription, settings);
            steady_clock::time_point endRun = steady_clock::now();
            fflush(stdout);

            double runDuration = duration<double>(endRun - startRun).count();
            std::cout << "Throughput of "
                      << toStringWithCommas(settings.numberOfMessages / runDuration)
                      << " RTTs/sec" << std::endl;
        }
        while (running && continuationBarrier("Execute again?"));
        running = false;

        pongThread.join();
    }
    catch (const SourcedException& e)
    {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }
    return EXIT_SUCCESS;
}