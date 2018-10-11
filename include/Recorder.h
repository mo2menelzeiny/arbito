
#ifndef ARBITO_RECORDER_H
#define ARBITO_RECORDER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>
#include <date/date.h>

// Disruptor
#include <Disruptor/Disruptor.h>
#include <Disruptor/BusySpinWaitStrategy.h>

// SpdLog
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

// Domain
#include "BusinessEvent.h"
#include "TradeEvent.h"
#include "RemoteMarketDataEvent.h"
#include "MarketDataEvent.h"
#include "ControlEvent.h"
#include "SystemEventType.h"
#include "SystemEvent.h"

class Recorder {
public:
	Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
		         const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		         const char *uri_string, int broker_num, const char *db_name);

	void start();

	BusinessState businessState();

	void systemEvent(const char *message, SystemEventType type);

private:

	void poll();

public:
	std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> m_trade_records_buffer;
	std::shared_ptr<Disruptor::RingBuffer<SystemEvent>> m_system_records_buffer;

private:
	const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> m_trade_buffer;
	const char *m_uri;
	const char *m_broker_name;
	const char *m_db_name;
	mongoc_client_t *m_mongoc_client;
	mongoc_collection_t *mongoc_coll_orders;
};


#endif //ARBITO_RECORDER_H
