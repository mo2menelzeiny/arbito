
#ifndef ARBITO_SWISSQUOTE_MARKETDATACLIENT_H
#define ARBITO_SWISSQUOTE_MARKETDATACLIENT_H

#define SWISSQUOTE_MO_MESSENGER_BUFFER 1024

// C
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

// SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Libtrading
#include <libtrading/proto/swissquote_common.h>
#include <libtrading/die.h>
#include <libtrading/array.h>

// Disruptor
#include <Disruptor/Disruptor.h>

// Aeron client
#include <Aeron.h>
#include <Context.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <FragmentAssembler.h>

// Aeron media driver
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "MarketDataEvent.h"
#include "ArbitrageDataEvent.h"
#include "Messenger.h"
#include "MessengerConfig.h"
#include "BrokerConfig.h"
#include "swissquote/Utilities.h"
#include "Recorder.h"

namespace SWISSQUOTE {

	class MarketOffice {

	public:
		MarketOffice(Recorder &recorder, Messenger &messenger,
		             const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_ringbuff,
		             const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &remote_md_ringbuff,
		             MessengerConfig messenger_config, BrokerConfig broker_config, double spread, double lot_size);

		void start();

	private:

		void initMessengerChannel();

		void initBrokerClient();

		void poll();

	private:
		double m_lot_size;
		double m_spread;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct swissquote_fix_session_cfg m_cfg;
		struct swissquote_fix_session *m_session;
		std::thread poller;
		BrokerConfig m_broker_config;
		MessengerConfig m_messenger_config;
		Messenger *m_messenger;
		std::shared_ptr<aeron::Publication> m_messenger_pub;
		std::shared_ptr<aeron::Subscription> m_messenger_sub;
		uint8_t m_buffer[SWISSQUOTE_MO_MESSENGER_BUFFER];
		aeron::concurrent::AtomicBuffer m_atomic_buffer;
		const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_ringbuffer;
		const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_remote_md_ringbuffer;
		Recorder *m_recorder;
	};
}


#endif //ARBITO_SWISSQUOTE_MARKETDATACLIENT_H
