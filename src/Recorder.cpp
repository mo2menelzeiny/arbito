
#include "Recorder.h"

using namespace date;
using namespace std::chrono;

Recorder::Recorder(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<TradeEvent>> &trade_buffer,
                   const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
                   const char *uri_string, int broker_number, const char *db_name)
		: m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer), m_business_buffer(business_buffer),
		  m_trade_buffer(trade_buffer), m_control_buffer(control_buffer), m_uri(uri_string), m_db_name(db_name) {

	m_local_records_buffer = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
			[]() { return MarketDataEvent(); }, 64, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_remote_records_buffer = Disruptor::RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
			[]() { return RemoteMarketDataEvent(); }, 64, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_business_records_buffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
			[]() { return BusinessEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_trade_records_buffer = Disruptor::RingBuffer<TradeEvent>::createSingleProducer(
			[]() { return TradeEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_control_records_buffer = Disruptor::RingBuffer<ControlEvent>::createMultiProducer(
			[]() { return ControlEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

	m_system_records_buffer = Disruptor::RingBuffer<SystemEvent>::createMultiProducer(
			[]() { return SystemEvent(); }, 32, std::make_shared<Disruptor::BusySpinWaitStrategy>());

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
	mongoc_init();

	bson_error_t error;
	auto uri = mongoc_uri_new_with_error(m_uri, &error);

	if (!uri) {
		fprintf(stderr,
		        "Recorder: failed to parse URI: %s\n"
		        "error message:       %s\n",
		        m_uri,
		        error.message
		);
		return;
	}

	m_mongoc_client = mongoc_client_new(m_uri);
	mongoc_coll_orders = mongoc_client_get_collection(m_mongoc_client, m_db_name, "coll_orders");

	auto poller = std::thread(&Recorder::poll, this);
	poller.detach();
}

BusinessState Recorder::businessState() {
	mongoc_client_t *client = mongoc_client_new(m_uri);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, m_db_name, "coll_orders");
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

void Recorder::systemEvent(const char *message, SystemEventType type) {
	auto next = m_system_records_buffer->next();
	(*m_system_records_buffer)[next].type = type;
	strcpy((*m_system_records_buffer)[next].message, message);
	m_system_records_buffer->publish(next);
}

void Recorder::poll() {
	pthread_setname_np(pthread_self(), "recorder");

	auto business_records_poller = m_business_records_buffer->newPoller();
	m_business_records_buffer->addGatingSequences({business_records_poller->sequence()});
	auto business_records_handler = [&](BusinessEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		OpenSide open_side = data.open_side;

		char clOrdId[64];
		sprintf(clOrdId, "%i", data.clOrdId);

		bson_t *insert = BCON_NEW (
				"timestamp_ms", BCON_DATE_TIME(data.timestamp_ms),
				"broker_name", BCON_UTF8(m_broker_name),
				"clOrdId", BCON_UTF8(clOrdId),
				"open_side", BCON_INT32(data.orders_count > 0 ? open_side : NONE),
				"orders_count", BCON_INT32(data.orders_count)
		);

		switch (data.side) {
			case '1':
				BSON_APPEND_UTF8(insert, "side", "BUY");
				break;

			case '2':
				BSON_APPEND_UTF8(insert, "side", "SELL");
				break;

			default:
				fprintf(stderr, "Recorder: Invalid side value\n");
				BSON_APPEND_UTF8(insert, "side", "FAILED");
				break;
		}

		switch (open_side) {
			case OPEN_BUY:
				if (data.side == '1') {
					BSON_APPEND_UTF8(insert, "order_type", "OPEN");
					systemEvent("BusinessOffice: OPEN", SE_TYPE_SUCCESS);
				} else {
					BSON_APPEND_UTF8(insert, "order_type", "CLOSE");
					systemEvent("BusinessOffice: CLOSE", SE_TYPE_SUCCESS);
				}
				break;
			case OPEN_SELL:
				if (data.side == '2') {
					BSON_APPEND_UTF8(insert, "order_type", "OPEN");
					systemEvent("BusinessOffice: OPEN", SE_TYPE_SUCCESS);
				} else {
					BSON_APPEND_UTF8(insert, "order_type", "CLOSE");
					systemEvent("BusinessOffice: CLOSE", SE_TYPE_SUCCESS);
				}
				break;
			case NONE:
				fprintf(stderr, "Recorder: Invalid side value\n");
				BSON_APPEND_UTF8(insert, "order_type", "FAILED");
				break;
		}

		BSON_APPEND_DOUBLE(insert, "trigger_px", data.trigger_px);
		BSON_APPEND_DOUBLE(insert, "remote_px", data.remote_px);

		if (!mongoc_collection_insert_one(mongoc_coll_orders, insert, nullptr, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(insert);
		return false;
	};

	auto trade_records_poller = m_trade_records_buffer->newPoller();
	m_trade_records_buffer->addGatingSequences({trade_records_poller->sequence()});
	auto trade_records_handler = [&](TradeEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		bson_error_t error;
		bson_t *selector = BCON_NEW ("clOrdId", data.clOrdId);
		bson_t *update_or_insert = BCON_NEW (
				"$set", "{",
				"exec_timestamp_ms", BCON_DATE_TIME(data.timestamp_ms),
				"orderId", BCON_UTF8(data.orderId),
				"avgPx", BCON_DOUBLE(data.avgPx),
				"}"
		);

		bson_t *opts = BCON_NEW ("upsert", BCON_BOOL(true));

		if (!mongoc_collection_update_one(mongoc_coll_orders, selector, update_or_insert, opts, nullptr, &error)) {
			fprintf(stderr, "Recorder: %s\n", error.message);
		}

		bson_destroy(update_or_insert);
		bson_destroy(opts);
		return false;
	};

	auto latency_logger = spdlog::daily_logger_st("Latency", "latency_log");
	auto remote_logger = spdlog::daily_logger_st("Remote", "remote_log");
	auto remote_records_poller = m_remote_records_buffer->newPoller();
	m_remote_records_buffer->addGatingSequences({remote_records_poller->sequence()});
	auto remote_records_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		milliseconds recv_duration(data.rec_timestamp_ms), ts_duration(data.timestamp_ms);
		time_point<system_clock> recv_tp(recv_duration), ts_tp(ts_duration);
		std::stringstream recv_ss, ts_ss;
		recv_ss << format("%T", time_point_cast<milliseconds>(recv_tp));
		ts_ss << format("%T", time_point_cast<milliseconds>(ts_tp));
		remote_logger->info(
				"offer: {} bid: {} remote: {} timestamp: {}",
				data.offer, data.bid, ts_ss.str(), recv_ss.str()
		);
		remote_logger->flush();
		latency_logger->info("{}ms", data.rec_timestamp_ms - data.timestamp_ms);
		latency_logger->flush();
		return false;
	};

	auto local_logger = spdlog::daily_logger_st("Local", "local_log");
	auto local_records_poller = m_local_records_buffer->newPoller();
	m_local_records_buffer->addGatingSequences({local_records_poller->sequence()});
	auto local_records_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		milliseconds ts_duration(data.timestamp_ms);
		time_point<system_clock> ts_tp(ts_duration);
		std::stringstream ts_ss;
		ts_ss << format("%T", time_point_cast<milliseconds>(ts_tp));
		local_logger->info(
				"offer: {} bid: {} broker: {} timestamp: {}",
				data.offer, data.bid, data.sending_time, ts_ss.str()
		);
		local_logger->flush();
		return false;
	};

	auto system_logger = spdlog::stdout_logger_st("System");
	auto system_records_poller = m_system_records_buffer->newPoller();
	m_system_records_buffer->addGatingSequences({system_records_poller->sequence()});
	auto system_records_handler = [&](SystemEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case SE_TYPE_ERROR:
				system_logger->error("{}", data.message);
				break;
			case SE_TYPE_SUCCESS:
				system_logger->info("{}", data.message);
				break;
		}
		system_logger->flush();
		return false;
	};

	auto control_records_poller = m_control_records_buffer->newPoller();
	m_control_records_buffer->addGatingSequences({control_records_poller->sequence()});
	auto control_records_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		milliseconds ts_duration(data.timestamp_ms);
		time_point<system_clock> ts_tp(ts_duration);
		std::stringstream ts_ss;
		ts_ss << format("%T", time_point_cast<milliseconds>(ts_tp));
		system_logger->info("Control event: type: {} timestamp: {}", data.type, ts_ss.str());
		system_logger->flush();
		return false;
	};

	while (true) {
		remote_records_poller->poll(remote_records_handler);
		local_records_poller->poll(local_records_handler);
		business_records_poller->poll(business_records_handler);
		trade_records_poller->poll(trade_records_handler);
		system_records_poller->poll(system_records_handler);
		control_records_poller->poll(control_records_handler);
		std::this_thread::yield();
	}
}
