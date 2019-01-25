
#include "MongoDBDriver.h"

MongoDBDriver::MongoDBDriver(
		const char *uri,
		const char *dbName,
		const char *collectionName
) : m_uri(uri),
    m_DBName(dbName),
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
}


void MongoDBDriver::record(const char *clOrdId, double bid, double offer, const char *orderType) {
	bson_error_t error;

	mongoc_client_t *client = mongoc_client_new(m_uri);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, m_DBName, m_collectionName);

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();

	bson_t *insert = BCON_NEW (
			"timestamp", BCON_DATE_TIME(nowMs),
			"clOrdId", BCON_UTF8(clOrdId),
			"orderType", BCON_UTF8(orderType),
			"bid", BCON_DOUBLE(bid),
			"offer", BCON_DOUBLE(offer)
	);

	if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
		auto consoleLogger = spdlog::get("console");
		consoleLogger->error("MongoDBDriver {}", error.message);
	}

	bson_destroy(insert);
	mongoc_client_destroy(client);
	mongoc_collection_destroy(collection);
}


void MongoDBDriver::record(
		const char *clOrdId,
		const char *orderId,
		char side,
		double fillPrice,
		const char *broker,
		bool isFilled
) {
	bson_error_t error;

	mongoc_client_t *client = mongoc_client_new(m_uri);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, m_DBName, m_collectionName);

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();

	bson_t *insert = BCON_NEW (
			"timestamp", BCON_DATE_TIME(nowMs),
			"broker", BCON_UTF8(broker),
			"clOrdId", BCON_UTF8(clOrdId),
			"orderId", BCON_UTF8(orderId),
			"side", BCON_UTF8(side == '1' ? "BUY" : "SELL"),
			"fillPrice", BCON_DOUBLE(fillPrice),
			"isFilled", BCON_BOOL(isFilled)
	);

	if (!mongoc_collection_insert_one(collection, insert, nullptr, nullptr, &error)) {
		auto consoleLogger = spdlog::get("console");
		consoleLogger->error("MongoDBDriver {}", error.message);
	}

	bson_destroy(insert);
	mongoc_client_destroy(client);
	mongoc_collection_destroy(collection);
}