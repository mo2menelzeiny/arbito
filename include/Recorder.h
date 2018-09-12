
#ifndef ARBITO_RECORDER_H
#define ARBITO_RECORDER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>

// Disruptor
#include <Disruptor/Disruptor.h>

// Domain
#include "BusinessEvent.h"
#include "TradeEvent.h"
#include "RemoteMarketDataEvent.h"
#include "MarketDataEvent.h"

enum SystemRecordType {
	SYSTEM_RECORD_TYPE_ERROR = 0,
	SYSTEM_RECORD_TYPE_SUCCESS = 1,
};

class Recorder {
public:
	Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
	         const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
	         const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
	         const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
	         const char *uri_string, int broker_num, const char *db_name);

	void recordSystem(const char *message, SystemRecordType type);

	void start();

private:

	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &m_remote_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> m_trade_buffer;
	mongoc_uri_t *m_uri;
	mongoc_client_pool_t *m_pool;
	const char *m_broker_name;
	const char *m_db_name;
	std::thread m_poller;
};


#endif //ARBITO_RECORDER_H
