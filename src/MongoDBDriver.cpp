
#include "MongoDBDriver.h"

MongoDBDriver::MongoDBDriver(
		const char *brokerName,
		const char *uri,
		const char *DBname,
		const char *collectionName
) : m_broker(brokerName),
    m_uri(uri),
    m_DBName(DBname),
    m_collectionName(collectionName) {
	bson_error_t error;

	mongoc_init();

	auto mongoCUri = mongoc_uri_new_with_error(m_uri, &error);

	if (!mongoCUri) {
		fprintf(stderr,
		        "MongoDB failed to parse URI: %s\n"
		        "error message:       %s\n",
		        m_uri,
		        error.message
		);
		return;
	}

	m_client = mongoc_client_new(m_uri);
	m_collection = mongoc_client_get_collection(m_client, m_DBName, collectionName);
}


void MongoDBDriver::recordTrigger(const char *clOrdId, double triggerPrice, const char *orderType) {
	bson_error_t error;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();
	bson_t *insert = BCON_NEW (
			"timestamp_ms", BCON_DATE_TIME(nowMs),
			"clOrdId", BCON_UTF8(clOrdId),
			"order_type", BCON_UTF8(orderType),
			"trigger_px", BCON_DOUBLE(triggerPrice)
	);

	if (!mongoc_collection_insert_one(m_collection, insert, nullptr, nullptr, &error)) {
		fprintf(stderr, "MongoDB %s\n", error.message);
	}

	bson_destroy(insert);
}


void MongoDBDriver::recordExecution(const char *clOrdId, const char *orderId, char side, double fillPrice) {
	bson_error_t error;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();

	bson_t *selector = BCON_NEW ("clOrdId", clOrdId);
	bson_t *update_or_insert = BCON_NEW (
			"$set", "{",
			"avgPx", BCON_DOUBLE(fillPrice),
			"side", BCON_UTF8(side == '1' ? "BUY" : "SELL"),
			"orderId", BCON_UTF8(orderId),
			"broker_name", BCON_UTF8(m_broker),
			"exec_timestamp_ms", BCON_DATE_TIME(nowMs),
			"}"
	);

	bson_t *opts = BCON_NEW ("upsert", BCON_BOOL(true));

	if (!mongoc_collection_update_one(m_collection, selector, update_or_insert, opts, nullptr, &error)) {
		fprintf(stderr, "MongoDB %s\n", error.message);
	}

	bson_destroy(update_or_insert);
	bson_destroy(opts);
}