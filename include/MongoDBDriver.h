
#ifndef ARBITO_MONGODBDRIVER_H
#define ARBITO_MONGODBDRIVER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>
#include <date/date.h>


class MongoDBDriver {

public:
	MongoDBDriver(
			const char *brokerName,
			const char *uri,
			const char *DBname,
			const char *collectionName
	);

	void recordTrigger(const char *clOrdId, double triggerPrice, const char *orderType);

	void recordExecution(const char *clOrdId, const char *orderId, char side, double fillPrice);



private:
	const char *m_broker;
	const char *m_uri;
	const char *m_DBName;
	const char *m_collectionName;
	mongoc_client_t *m_client;
	mongoc_collection_t *m_collection;
};


#endif //ARBITO_MONGODBDRIVER_H
