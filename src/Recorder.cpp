
#include "Recorder.h"

Recorder::Recorder(const char *uri_string, int broker_number, const char *db_name) : m_db_name(db_name) {
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
	ping();

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

int Recorder::ping() {
	bson_error_t error;
	mongoc_client_t *client;
	bson_t ping = BSON_INITIALIZER;
	bool r;

	BSON_APPEND_INT32 (&ping, "ping", 1);

	client = mongoc_client_pool_pop(m_pool);

	r = mongoc_client_command_simple(
			client, "db_arbito", &ping, NULL, NULL, &error);

	if (!r) {
		fprintf(stderr, "Recorder: %s\n", error.message);
		return 0;
	}

	mongoc_client_pool_push(m_pool, client);
	bson_destroy(&ping);
	return 1;
}

void Recorder::recordSystem(const char *message, SystemRecordType type) {
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
	mongoc_collection_t *coll_system = mongoc_client_get_collection(client, m_db_name, "coll_system");
	auto milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
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

	if (!mongoc_collection_insert_one(coll_system, insert, NULL, NULL, &error)) {
		fprintf(stderr, "Recorder: %s\n", error.message);
	}

	bson_destroy(insert);
	mongoc_client_pool_push(m_pool, client);
}

void Recorder::recordArbitrage(ArbitrageDataEvent &event) {
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
	mongoc_collection_t *coll_system = mongoc_client_get_collection(client, m_db_name, "coll_arbitrage");
	auto milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
	bson_t *insert = BCON_NEW (
			"broker_name", BCON_UTF8(m_broker_name),
			"created_at", BCON_DATE_TIME(milliseconds_since_epoch),
			"timestamp", BCON_UTF8(event.timestamp),
			"l1", "{",
			"bid", BCON_DOUBLE(event.l1.bid),
			"bid_qty", BCON_DOUBLE(event.l1.bid_qty),
			"offer", BCON_DOUBLE(event.l1.offer),
			"offer_qty", BCON_DOUBLE(event.l1.offer_qty),
			"timestamp", BCON_UTF8(event.l1.timestamp),
			"}",
			"l2", "{",
			"bid", BCON_DOUBLE(event.l2.bid),
			"bid_qty", BCON_DOUBLE(event.l2.bid_qty),
			"offer", BCON_DOUBLE(event.l2.offer),
			"offer_qty", BCON_DOUBLE(event.l2.offer_qty),
			"timestamp", BCON_UTF8(event.l2.timestamp),
			"}"
	);

	if (!mongoc_collection_insert_one(coll_system, insert, NULL, NULL, &error)) {
		fprintf(stderr, "Recorder: %s\n", error.message);
	}

	bson_destroy(insert);
	mongoc_client_pool_push(m_pool, client);
}

void Recorder::recordOrder(double broker_price, double trigger_price, OrderRecordType order_type, double trigger_diff,
                           OrderTriggerType trigger_type, OrderRecordState order_state) {
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_pool_pop(m_pool);
	mongoc_collection_t *coll_system = mongoc_client_get_collection(client, m_db_name, "coll_orders");
	auto milliseconds_since_epoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
	bson_t *insert = BCON_NEW (
			"broker_name", BCON_UTF8(m_broker_name),
			"created_at", BCON_DATE_TIME(milliseconds_since_epoch),
			"trigger_diff", BCON_DOUBLE(trigger_diff)
	);

	switch (order_type) {
		case ORDER_RECORD_TYPE_BUY:
			BSON_APPEND_DOUBLE(insert, "slippage", trigger_price - broker_price);
			BSON_APPEND_UTF8(insert, "order_type", "BUY");
			break;
		case ORDER_RECORD_TYPE_SELL:
			BSON_APPEND_DOUBLE(insert, "slippage", broker_price - trigger_price);
			BSON_APPEND_UTF8(insert, "order_type", "SELL");
			break;
	}

	switch (order_state) {
		case ORDER_RECORD_STATE_OPEN:
			BSON_APPEND_UTF8(insert, "order_state", "OPEN");
			break;
		case ORDER_RECORD_STATE_CLOSE:
			BSON_APPEND_UTF8(insert, "order_state", "CLOSE");
			break;
		case ORDER_RECORD_STATE_INIT:
			BSON_APPEND_UTF8(insert, "order_state", "INITIAL OPEN");
	}

	switch (trigger_type) {
		case ORDER_TRIGGER_TYPE_CURRENT_DIFF_2:
			BSON_APPEND_UTF8(insert, "order_trigger", "CURRENT DIFF 2");
			break;
		case ORDER_TRIGGER_TYPE_CURRENT_DIFF_1:
			BSON_APPEND_UTF8(insert, "order_trigger", "CURRENT DIFF 1");
			break;
	}

	if (!mongoc_collection_insert_one(coll_system, insert, NULL, NULL, &error)) {
		fprintf(stderr, "Recorder: %s\n", error.message);
	}

	bson_destroy(insert);
	mongoc_client_pool_push(m_pool, client);
}

