
#ifndef ARBITO_SWISSQUOTE_TRADEOFFICE_H
#define ARBITO_SWISSQUOTE_TRADEOFFICE_H

#define MAX_DEALS 5
#define DELAY_SECONDS 60

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
#include <stack>

// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Libtrading includes
#include <libtrading/proto/swissquote_common.h>
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
#include "Utilities.h"
#include "ArbitrageDataEvent.h"
#include "Messenger.h"

namespace SWISSQUOTE {

	enum MarketState {
		NO_DEALS = 0,
		CURRENT_DIFFERENCE_1 = 1,
		CURRENT_DIFFERENCE_2 = 2
	};

	class TradeOffice {

	public:
		TradeOffice(const std::shared_ptr<Messenger> &messenger,
		            const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> &arbitrage_data_disruptor,
		            const char *m_host, int m_port, const char *username, const char *password,
		            const char *sender_comp_id, const char *target_comp_id, int heartbeat,
		            double diff_open, double diff_close, double bid_lot_size, double offer_lot_size);

		void start();

	private:
		void initBrokerClient();

		void poll();

	private:
		MarketState m_open_state = NO_DEALS;
		double m_diff_open, m_diff_close;
		double m_bid_lot_size, m_offer_lot_size;
		int m_deals_count = 0;
		int m_port;
		const char *m_host;
		SSL_CTX *m_ssl_ctx;
		SSL *m_ssl;
		struct swissquote_fix_session_cfg m_cfg;
		struct swissquote_fix_session *m_session;
		std::thread poller;
		const std::shared_ptr<Messenger> m_messenger;
		const std::shared_ptr<Disruptor::disruptor<ArbitrageDataEvent>> m_arbitrage_data_disruptor;
	};
}

#endif //ARBITO_SWISSQUOTE_TRADEOFFICE_H