
#ifndef ARBITO_RECORDER_H
#define ARBITO_RECORDER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>

// Disruptor
#include <Disruptor/Disruptor.h>
#include <Disruptor/BusySpinWaitStrategy.h>

// Domain
#include "BusinessEvent.h"
#include "TradeEvent.h"
#include "RemoteMarketDataEvent.h"
#include "MarketDataEvent.h"
#include "ControlEvent.h"
#include "SystemRecordType.h"

class Recorder {
public:
	Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		         const char *uri_string, int broker_num, const char *db_name);

	void start();

	BusinessState fetchBusinessState();

	void recordSystemMessage(const char *message, SystemRecordType type);

private:

	void poll();

public:
	std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> m_trade_records_buffer;

private:
	const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> m_trade_buffer;
	mongoc_uri_t *m_uri;
	mongoc_client_pool_t *m_pool;
	const char *m_broker_name;
	const char *m_db_name;
	std::thread m_poller;
};


#endif //ARBITO_RECORDER_H
