
#include "Recorder.h"

Recorder::Recorder(const char *uri_string, int broker_number) {
	bson_error_t error;

	mongoc_init();

	m_uri = mongoc_uri_new_with_error (uri_string, &error);

	if (!m_uri) {
		fprintf (stderr,
		         "failed to parse URI: %s\n"
		         "error message:       %s\n",
		         uri_string,
		         error.message);
		return;
	}

	m_pool = mongoc_client_pool_new(m_uri);
	mongoc_client_pool_set_error_api (m_pool, 2);
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

	r = mongoc_client_command_simple (
			client, "db_arbito", &ping, NULL, NULL, &error);

	if (!r) {
		fprintf (stderr, "%s\n", error.message);
		return 0;
	}

	mongoc_client_pool_push (m_pool, client);
	bson_destroy (&ping);
	return 1;
}

void Recorder::recordSystemMessage(const char *message, SystemRecordType type) {
	bson_error_t error;
	mongoc_client_t *client = mongoc_client_pool_pop (m_pool);
	mongoc_collection_t *coll_system = mongoc_client_get_collection (client, "db_arbito", "coll_system");
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

	if (!mongoc_collection_insert_one (coll_system, insert, NULL, NULL, &error)) {
		fprintf (stderr, "%s\n", error.message);
	}

	bson_destroy (insert);
	mongoc_client_pool_push (m_pool, client);
}


