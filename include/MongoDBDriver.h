
#ifndef ARBITO_MONGODBDRIVER_H
#define ARBITO_MONGODBDRIVER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>

class MongoDBDriver {

public:
	MongoDBDriver(
			const char *uri,
			const char *dbName,
			const char *collectionName
	);

	void record(unsigned long clOrdId, double bid, double offer, const char *orderType);

	void record(unsigned long clOrdId, const char *orderId, char side, double fillPrice, const char *broker);



private:
	const char *m_uri;
	const char *m_DBName;
	const char *m_collectionName;
	mongoc_client_t *m_client;
	mongoc_collection_t *m_collection;
};


#endif //ARBITO_MONGODBDRIVER_H
