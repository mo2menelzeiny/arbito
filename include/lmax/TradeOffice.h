
#ifndef ARBITO_LMAX_TRADEOFFICE_H
#define ARBITO_LMAX_TRADEOFFICE_H

#define AERON_BUFFER_SIZE 1024
#define MAX_DEALS 5

// C includes
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
#include <stdexcept>

// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Libtrading includes
#include <libtrading/proto/lmax_common.h>
#include <libtrading/die.h>
#include <libtrading/array.h>

// Disruptor includes
#include <Disruptor/Disruptor.h>
#include <Disruptor/ThreadPerTaskScheduler.h>
#include <Disruptor/BusySpinWaitStrategy.h>
#include <Disruptor/RingBuffer.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ILifecycleAware.h>

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <FragmentAssembler.h>

// Aeron media driver
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain includes
#include <Event.h>
#include <MarketDataEvent.h>
#include <stack>

namespace LMAX {

	enum MarketState {
		NO_DEALS = 0,
		CURRENT_DIFFERENCE_ONE = 1,
		CURRENT_DIFFERENCE_TWO = 2
	};

	struct Deal {
		unsigned long id;
		double slip;
	};

	// struct to contain the Aeron configurations parameters
	struct AeronConfig {
		const char *pub_channel;
		int pub_stream_id;
		const char *sub_channel;
		int sub_stream_id;
	};

	class TradeOffice : public Disruptor::IEventHandler<MarketDataEvent> {

	public:
		TradeOffice(const char *m_host, int m_port, const char *username, const char *password,
		            const char *sender_comp_id, const char *target_comp_id, int heartbeat,
		            const char *pub_channel, const char *pub_stream_id, const char *sub_channel,
		            const char *sub_stream_id, double diff_open, double diff_close);

		template<class T>
		void addConsumer(const std::shared_ptr<T> consumer) {
			m_disruptor->handleEventsWith(consumer);
		};

		void start();

		void onEvent(MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) override;

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

		void initBrokerClient();

		void pollBrokerClient();

		void messengerMediaDriver();

		void initMessengerClient();

		void pollMessengerClient();

		void onMessengerMarketData(sbe::MarketData &marketData);

	private:
		int m_port;
		const char *m_host;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct lmax_fix_session_cfg m_cfg;
		struct lmax_fix_session *m_session;
		std::shared_ptr<Disruptor::disruptor<Event>> m_disruptor;
		std::shared_ptr<Disruptor::ThreadPerTaskScheduler> m_taskscheduler;
		AeronConfig m_aeron_config;
		aeron::Context m_aeron_context;
		std::shared_ptr<aeron::Aeron> m_aeron;
		std::shared_ptr<aeron::Publication> m_publication;
		std::shared_ptr<aeron::Subscription> m_subscription;
		std::thread m_broker_client_poller;
		std::thread m_media_driver;
		std::thread m_messenger_client_poller;
		uint8_t m_buffer[AERON_BUFFER_SIZE];
		MarketDataEvent *last_market_data = nullptr;
		double m_diff_open, m_diff_close;
		double m_current_diff_one, m_current_diff_two;
		MarketState m_market_state = NO_DEALS;
		std::stack<Deal> deals;
	};
}

#endif //ARBITO_LMAX_TRADEOFFICE_H
