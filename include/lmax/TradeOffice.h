
#ifndef ARBITO_LMAX_TRADEOFFICE_H
#define ARBITO_LMAX_TRADEOFFICE_H

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
#include <deque>
#include <chrono>

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

// Domain includes
#include "Config.h"
#include "Messenger.h"
#include "Recorder.h"
#include "BrokerConfig.h"
#include "TradeEvent.h"
#include "BusinessEvent.h"
#include "ControlEvent.h"
#include "lmax/Utilities.h"

using namespace Disruptor;
using namespace std;
using namespace chrono;

namespace LMAX {

	class TradeOffice {

	public:
		TradeOffice(
				const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
				const shared_ptr<RingBuffer<BusinessEvent>> &business_buffer,
				const shared_ptr<RingBuffer<TradeEvent>> &trade_buffer,
				Recorder &recorder,
				BrokerConfig broker_config,
				double lot_size
		);

		void start();

	private:
		bool connectToBroker();

		void poll();

	private:
		const shared_ptr<RingBuffer<ControlEvent>> m_control_buffer;
		const shared_ptr<RingBuffer<BusinessEvent>> m_business_buffer;
		const shared_ptr<RingBuffer<TradeEvent>> m_trade_buffer;
		Recorder *m_recorder;
		BrokerConfig m_broker_config;
		double m_lot_size;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct lmax_fix_session_cfg m_cfg;
		struct lmax_fix_session *m_session;
	};
}

#endif //ARBITO_LMAX_TRADEOFFICE_H
