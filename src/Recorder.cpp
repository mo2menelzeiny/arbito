
#include "Recorder.h"

Recorder::Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
                   const char *uri_string, int broker_number, const char *db_name)
		: m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer), m_business_buffer(business_buffer),
		  m_trade_buffer(trade_buffer), m_control_buffer(control_buffer), m_uri(uri_string), m_db_name(db_name) {

	m_local_records_buffer = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
			[]() { return MarketDataEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_remote_records_buffer = Disruptor::RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
			[]() { return RemoteMarketDataEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_business_records_buffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
			[]() { return BusinessEvent(); }, 16, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_trade_records_buffer = Disruptor::RingBuffer<TradeEvent>::createSingleProducer(
			[]() { return TradeEvent(); }, 16, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_control_records_buffer = Disruptor::RingBuffer<ControlEvent>::createMultiProducer(
			[]() { return ControlEvent(); }, 16, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	mongoc_init();

	switch (broker_number) {
		case 1:
			m_broker_name = "LMAX";
			break;
		case 2:
			m_broker_name = "SWISSQUOTE";
			break;
		default:
			fprintf(stderr, "Recorder: undefined broker number\n");
	}
}

void Recorder::start() {
	bson_error_t error;
	auto uri = mongoc_uri_new_with_error(m_uri, &error);

	if (!uri) {
		fprintf(stderr,
		        "Recorder: failed to parse URI: %s\n"
		        "error message:       %s\n",
		        m_uri,
		        error.message);
		return;
	}

	m_poller = std::thread(&Recorder::poll, this);
	m_poller.detach();
}

BusinessState Recorder::fetchBusinessState() {
	mongoc_client_t *client = mongoc_client_new(m_uri);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_business_events");
	mongoc_cursor_t *cursor;
	const bson_t *doc;
	bson_iter_t iter;
	BusinessState business_state = (BusinessState) {
			.open_side = NONE,
			.orders_count = 0
	};

	bson_t *filter = BCON_NEW ("broker_name", m_broker_name);
	bson_t *opts = BCON_NEW("limit", BCON_INT32(1), "sort", "{", "$natural", BCON_INT32(-1), "}");

	cursor = mongoc_collection_find_with_opts(collection, filter, opts, nullptr);

	if (mongoc_cursor_next(cursor, &doc)) {

		if (bson_iter_init_find(&iter, doc, "open_side")) {
			business_state.open_side = static_cast<OpenSide>(bson_iter_int32(&iter));
		}

		if (bson_iter_init_find(&iter, doc, "orders_count")) {
			business_state.orders_count = bson_iter_int32(&iter);
		}
	};

	bson_destroy(filter);
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy(collection);
	mongoc_client_destroy(client);
	return business_state;
}

void Recorder::recordSystemMessage(const char *message, SystemRecordType type) {
	auto milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_new(m_uri);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_system");
	bson_t *insert = BCON_NEW (
			"broker_name", BCON_UTF8(m_broker_name),
			"message", BCON_UTF8(message),
			"created_at", BCON_DATE_TIME(milliseconds_since_epoch)
	);

	switch (type) {
		case SYSTEM_RECORD_TYPE_SUCCESS:
			BSON_APPEND_UTF8(insert, "message_type", "SUCCESS");
			break;
		case SYSTEM_RECORD_TYPE_ERROR:
			BSON_APPEND_UTF8(insert, "message_type", "ERROR");
			break;
	}

	if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
		fprintf(stderr, "Recorder: %s\n", error.message);
	}

	bson_destroy(insert);
	mongoc_collection_destroy(collection);
	mongoc_client_destroy(client);
}

void Recorder::poll() {
	pthread_setname_np(pthread_self(), "recorder");

	auto business_records_poller = m_business_records_buffer->newPoller();
	m_business_records_buffer->addGatingSequences({business_records_poller->sequence()});
	auto business_records_handler = [&](BusinessEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_new(m_uri);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_business_events");

		OpenSide open_side = data.open_side;
		if (!data.orders_count) {
			open_side = NONE;
		}

		char clOrdId[64];
		sprintf(clOrdId, "%i", data.clOrdId);

		bson_t *insert_business_event = BCON_NEW (
				"timestamp_ms", BCON_DATE_TIME(data.timestamp_ms),
				"broker_name", BCON_UTF8(m_broker_name),
				"clOrdId", BCON_UTF8(clOrdId),
				"trigger_px", BCON_DOUBLE(data.trigger_px),
				"remote_px", BCON_DOUBLE(data.remote_px),
				"open_side", BCON_INT32(open_side),
				"orders_count", BCON_INT32(data.orders_count)
		);

		switch (data.side) {
			case '1':
				BSON_APPEND_UTF8(insert_business_event, "side", "BUY");
				break;

			case '2':
				BSON_APPEND_UTF8(insert_business_event, "side", "SELL");
				break;

			default:
				break;
		}

		if (!mongoc_collection_insert_one(collection, insert_business_event, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert_business_event);
		mongoc_collection_destroy(collection);
		mongoc_client_destroy(client);
		return false;
	};

	auto trade_records_poller = m_trade_records_buffer->newPoller();
	m_trade_records_buffer->addGatingSequences({trade_records_poller->sequence()});
	auto trade_records_handler = [&](TradeEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_new(m_uri);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_trade_events");
		bson_t *insert = BCON_NEW (
				"timestamp_ms", BCON_DATE_TIME(data.timestamp_ms),
				"broker_name", BCON_UTF8(m_broker_name),
				"clOrdId", BCON_UTF8(data.clOrdId),
				"orderId", BCON_UTF8(data.orderId),
				"avgPx", BCON_DOUBLE(data.avgPx)
		);

		switch (data.side) {
			case '1':
				BSON_APPEND_UTF8(insert, "side", "BUY");
				break;
			case '2':
				BSON_APPEND_UTF8(insert, "side", "SELL");
				break;
			default:
				BSON_APPEND_UTF8(insert, "side", "FAILED");
				break;
		}

		if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert);
		mongoc_collection_destroy(collection);
		mongoc_client_destroy(client);
		return false;
	};

	int latency_flush_counter = 0;
	long latency_max = 0, latency_min = 99, latency_tot = 0;
	auto latency_logger = spdlog::daily_logger_st("Latency", "latency_log");
	auto remote_records_poller = m_remote_records_buffer->newPoller();
	m_remote_records_buffer->addGatingSequences({remote_records_poller->sequence()});
	auto remote_records_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		++latency_flush_counter;
		long latency = data.rec_timestamp_ms - data.timestamp_ms;
		latency_tot += latency;

		if (latency_max < latency) {
			latency_max = latency;
		}

		if (latency_min > latency) {
			latency_min = latency;
		}

		if (latency_flush_counter >= 10) {
			long latency_avg = latency_tot / latency_flush_counter;
			latency_logger->info("Average: {}ms Min: {}ms Max: {}ms", latency_avg, latency_min, latency_max);
			latency_logger->flush();
			latency_tot = latency_min = latency_max =  0;
			latency_flush_counter = 0;
		}

		return false;
	};

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		remote_records_poller->poll(remote_records_handler);
		business_records_poller->poll(business_records_handler);
		trade_records_poller->poll(trade_records_handler);
	}
}
