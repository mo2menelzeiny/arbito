
#ifndef ARBITO_MARKETDATACLIENT_H
#define ARBITO_MARKETDATACLIENT_H

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
#include <Disruptor/BusySpinWaitStrategy.h>
#include <Disruptor/RingBuffer.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ILifecycleAware.h>

namespace LMAX {

    struct DataEvent {
        const char *value;
    };

    struct PrintEventHandler : Disruptor::IEventHandler<DataEvent> {

        void onEvent(DataEvent &data, std::int64_t sequence, bool endOfBatch) override;
    };

    class MarketDataClient {
    public:
        MarketDataClient(const char *m_host, int m_port, const char *username, const char *password,
                         const char *sender_comp_id, const char *target_comp_id, int heartbeat);
        void start();

        void stop();

        void handle();

        void onData(lmax_fix_message *msg);

    private:
        static int fstrncpy(char *dest, const char *src, int n) {
            int i;

            for (i = 0; i < n && src[i] != 0x00 && src[i] != 0x01; i++)
                dest[i] = src[i];

            return i;
        }

        static void fprintmsg(FILE *stream, struct lmax_fix_message *msg) {
            char buf[256];
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

        static int socket_setopt(int sockfd, int level, int optname, int optval) {
            return setsockopt(sockfd, level, optname, (void *) &optval, sizeof(optval));
        }

    private:
        int m_port;
        const char *m_host;
        SSL_CTX *m_ssl_ctx;
        SSL *m_ssl;
        struct lmax_fix_session_cfg m_cfg;
        struct lmax_fix_session *m_session;
        std::shared_ptr<Disruptor::disruptor<LMAX::DataEvent>> m_disruptor;
        std::shared_ptr<Disruptor::ThreadPerTaskScheduler> m_taskscheduler;
        char m_buffer[256];
    };
}


#endif //ARBITO_MARKETDATACLIENT_H
