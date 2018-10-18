
#ifndef ARBITO_LMAX_MARKETOFFICE_H
#define ARBITO_LMAX_MARKETOFFICE_H

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
#include <libtrading/proto/lmax_common.h>
#include <libtrading/die.h>
#include <libtrading/array.h>

// Disruptor
#include <Disruptor/Disruptor.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "Config.h"
#include "Recorder.h"
#include "Messenger.h"
#include "MarketDataEvent.h"
#include "ControlEvent.h"
#include "BrokerConfig.h"
#include "lmax/Utilities.h"

using namespace Disruptor;
using namespace std;
using namespace chrono;

namespace LMAX {

	class MarketOffice {

	public:
		MarketOffice(
				const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
				const shared_ptr<RingBuffer<MarketDataEvent>> &local_md_buffer,
				Recorder &recorder,
				BrokerConfig broker_config,
				double spread,
				double lot_size
		);

		void start();

	private:

		bool connectToBroker();

		void poll();

	private:
		const shared_ptr<RingBuffer<ControlEvent>> m_control_buffer;
		const shared_ptr<RingBuffer<MarketDataEvent>> m_local_md_buffer;
		Recorder *m_recorder;
		BrokerConfig m_broker_config;
		double m_lot_size;
		double m_spread;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct lmax_fix_session_cfg m_cfg;
		struct lmax_fix_session *m_session;
	};
}


#endif //ARBITO_LMAX_MARKETOFFICE_H
