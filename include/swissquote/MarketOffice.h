
#ifndef ARBITO_SWISSQUOTE_MARKETOFFICE_H
#define ARBITO_SWISSQUOTE_MARKETOFFICE_H

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

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "Config.h"
#include "Messenger.h"
#include "Recorder.h"
#include "BrokerConfig.h"
#include "MarketDataEvent.h"
#include "ControlEvent.h"
#include "swissquote/Utilities.h"

namespace SWISSQUOTE {

	class MarketOffice {

	public:
		MarketOffice(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		             const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
		             Recorder &recorder, BrokerConfig broker_config, double spread,
		             double lot_size);

		void start();

	private:
		void connectToBroker();

		void poll();

	private:
		const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
		const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
		Recorder *m_recorder;
		BrokerConfig m_broker_config;
		double m_lot_size;
		double m_spread;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct swissquote_fix_session_cfg m_cfg;
		struct swissquote_fix_session *m_session;
		std::thread m_poller;
	};
}


#endif //ARBITO_SWISSQUOTE_MARKETOFFICE_H
