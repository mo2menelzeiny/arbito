
#include "Recorder.h"

Recorder::Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
                   const char *uri_string, int broker_number, const char *db_name)
		: m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer), m_business_buffer(business_buffer),
		  m_trade_buffer(trade_buffer), m_control_buffer(control_buffer), m_db_name(db_name) {

	m_local_records_buffer = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
			[]() { return MarketDataEvent(); }, 128, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_remote_records_buffer = Disruptor::RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
			[]() { return RemoteMarketDataEvent(); }, 128, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_business_records_buffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
			[]() { return BusinessEvent(); }, 64, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_trade_records_buffer = Disruptor::RingBuffer<TradeEvent>::createSingleProducer(
			[]() { return TradeEvent(); }, 64, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_control_records_buffer = Disruptor::RingBuffer<ControlEvent>::createSingleProducer(
			[]() { return ControlEvent(); }, 64, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	bson_error_t error;
	mongoc_init();
	m_uri = mongoc_uri_new_with_error(uri_string, &error);

	if (!m_uri) {
		fprintf(stderr,
		        "Recorder: failed to parse URI: %s\n"
		        "error message:       %s\n",
		        uri_string,
		        error.message);
		return;
	}

	m_pool = mongoc_client_pool_new(m_uri);
	mongoc_client_pool_set_error_api(m_pool, 2);

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
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	m_buffers_poller = std::thread(&Recorder::pollBuffers, this);
	pthread_setaffinity_np(m_buffers_poller.native_handle(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(m_buffers_poller.native_handle(), "recorder-buff");
	m_buffers_poller.detach();

	m_records_poller = std::thread(&Recorder::pollRecords, this);
	pthread_setname_np(m_records_poller.native_handle(), "recorder-poll");
	m_records_poller.detach();

}

BusinessState Recorder::fetchBusinessState() {
	mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
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
	mongoc_client_pool_push(m_pool, client);
	return business_state;
}

void Recorder::recordSystemMessage(const char *message, SystemRecordType type) {
	auto milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
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
	mongoc_client_pool_push(m_pool, client);
}

void Recorder::pollBuffers() {
	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		try {
			auto next = m_control_records_buffer->tryNext();
			(*m_control_records_buffer)[next] = data;
			m_control_records_buffer->publish(next);
		} catch (Disruptor::InsufficientCapacityException &exception) {}

		return false;
	};

	auto business_poller = m_business_buffer->newPoller();
	m_business_buffer->addGatingSequences({business_poller->sequence()});
	auto business_handler = [&](BusinessEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		try {
			auto next = m_business_records_buffer->tryNext();
			(*m_business_records_buffer)[next] = data;
			m_business_records_buffer->publish(next);
		} catch (Disruptor::InsufficientCapacityException &exception) {}

		return false;
	};

	auto trade_poller = m_trade_buffer->newPoller();
	m_trade_buffer->addGatingSequences({trade_poller->sequence()});
	auto trade_handler = [&](TradeEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		try {
			auto next = m_trade_records_buffer->tryNext();
			(*m_trade_records_buffer)[next] = data;
			m_trade_records_buffer->publish(next);
		} catch (Disruptor::InsufficientCapacityException &exception) {}

		return false;
	};

	auto remote_md_poller = m_remote_md_buffer->newPoller();
	m_remote_md_buffer->addGatingSequences({remote_md_poller->sequence()});
	auto remote_md_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		try {
			auto next = m_remote_records_buffer->tryNext();
			(*m_remote_records_buffer)[next] = data;
			m_remote_records_buffer->publish(next);
		} catch (Disruptor::InsufficientCapacityException &exception) {}

		return false;
	};

	/*auto local_md_poller = m_local_md_buffer->newPoller();
	m_local_md_buffer->addGatingSequences({local_md_poller->sequence()});
	auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		try {
			auto next = m_local_records_buffer->tryNext();
			(*m_local_records_buffer)[next] = data;
			m_local_records_buffer->publish(next);
		} catch (Disruptor::InsufficientCapacityException &exception) {}

		return false;
	};*/

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		control_poller->poll(control_handler);
		// local_md_poller->poll(local_md_handler);
		remote_md_poller->poll(remote_md_handler);
		business_poller->poll(business_handler);
		trade_poller->poll(trade_handler);
	}
}

void Recorder::pollRecords() {
	auto control_records_poller = m_control_records_buffer->newPoller();
	m_control_records_buffer->addGatingSequences({control_records_poller->sequence()});
	auto control_records_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_control_events");

		bson_t *insert = BCON_NEW (
				"timestamp_us", BCON_DATE_TIME(data.timestamp_us / 1000),
				"broker_name", BCON_UTF8(m_broker_name),
				"source", BCON_INT32(data.source),
				"type", BCON_INT32(data.type)
		);

		if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert);
		mongoc_collection_destroy(collection);
		mongoc_client_pool_push(m_pool, client);
		return false;
	};

	auto business_records_poller = m_business_records_buffer->newPoller();
	m_business_records_buffer->addGatingSequences({business_records_poller->sequence()});
	auto business_records_handler = [&](BusinessEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_business_events");

		OpenSide open_side = data.open_side;
		if (!data.orders_count) {
			open_side = NONE;
		}

		char clOrdId[64];
		sprintf(clOrdId, "%i", data.clOrdId);

		bson_t *insert_business_event = BCON_NEW (
				"timestamp_us", BCON_DATE_TIME(data.timestamp_us / 1000),
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
		mongoc_client_pool_push(m_pool, client);
		return false;
	};

	auto trade_records_poller = m_trade_records_buffer->newPoller();
	m_trade_records_buffer->addGatingSequences({trade_records_poller->sequence()});
	auto trade_records_handler = [&](TradeEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_trade_events");
		bson_t *insert = BCON_NEW (
				"timestamp_us", BCON_DATE_TIME(data.timestamp_us / 1000),
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
		mongoc_client_pool_push(m_pool, client);
		return false;
	};

	auto remote_records_poller = m_remote_records_buffer->newPoller();
	m_remote_records_buffer->addGatingSequences({remote_records_poller->sequence()});
	auto remote_records_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_remote_md_events");
		bson_t *insert = BCON_NEW (
				"timestamp_us", BCON_DATE_TIME(data.timestamp_us / 1000),
				"rec_timestamp_us", BCON_DATE_TIME(data.rec_timestamp_us / 1000),
				"broker_name", BCON_UTF8(m_broker_name),
				"offer", BCON_DOUBLE(data.offer),
				"bid", BCON_DOUBLE(data.bid)
		);

		if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert);
		mongoc_collection_destroy(collection);
		mongoc_client_pool_push(m_pool, client);
		return false;
	};

	/*auto local_records_poller = m_local_records_buffer->newPoller();
	m_local_records_buffer->addGatingSequences({local_records_poller->sequence()});
	auto local_records_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
		mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_local_md_events");
		bson_t *insert = BCON_NEW (
				"timestamp_us", BCON_DATE_TIME(data.timestamp_us / 1000),
				"broker_name", BCON_UTF8(m_broker_name),
				"offer", BCON_DOUBLE(data.offer),
				"bid", BCON_DOUBLE(data.bid)
		);

		if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert);
		mongoc_collection_destroy(collection);
		mongoc_client_pool_push(m_pool, client);
		return false;
	};*/

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		control_records_poller->poll(control_records_handler);
		// local_records_poller->poll(local_records_handler);
		remote_records_poller->poll(remote_records_handler);
		business_records_poller->poll(business_records_handler);
		trade_records_poller->poll(trade_records_handler);
	}
}
