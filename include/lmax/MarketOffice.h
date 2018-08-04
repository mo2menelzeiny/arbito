
#ifndef ARBITO_LMAX_MARKETDATACLIENT_H
#define ARBITO_LMAX_MARKETDATACLIENT_H

#define MESSEGNER_BUFFER 1024

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
#include "BrokerMarketDataHandler.h"
#include "lmax/Utilities.h"

namespace LMAX {

	class MarketOffice {

	public:
		MarketOffice(const std::shared_ptr<Messenger> &messenger,
		             const std::shared_ptr<Disruptor::disruptor<MarketDataEvent>> &broker_market_data_disruptor,
		             const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> &arbitrage_data_disruptor,
		             const char *m_host, int m_port, const char *username,
		             const char *password, const char *sender_comp_id, const char *target_comp_id, int heartbeat,
		             const char *pub_channel, int pub_stream_id, const char *sub_channel,
		             int sub_stream_id, double spread, double bid_lot_size, double offer_lot_size);

		void start();

	private:

		void initMessengerChannel();

		void initBrokerClient();

		void poll();

	private:
		double m_bid_lot_size;
		double m_offer_lot_size;
		double m_spread;
		int m_port;
		const char *m_host;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct lmax_fix_session_cfg m_cfg;
		struct lmax_fix_session *m_session;
		std::thread poller;
		MessengerConfig m_messenger_config;
		const std::shared_ptr<Messenger> m_messenger;
		std::shared_ptr<aeron::Publication> m_messenger_pub;
		std::shared_ptr<aeron::Subscription> m_messenger_sub;
		const std::shared_ptr<Disruptor::disruptor<MarketDataEvent>> m_broker_market_data_disruptor;
		const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> m_arbitrage_data_disruptor;
		std::shared_ptr<BrokerMarketDataHandler> m_broker_market_data_handler;
	};
}


#endif //ARBITO_LMAX_MARKETDATACLIENT_H
