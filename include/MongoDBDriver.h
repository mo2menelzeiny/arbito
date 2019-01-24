
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

	void record(const char *clOrdId, double bid, double offer, const char *orderType);

	void record(
			const char *clOrdId,
			const char *orderId,
			char side,
			double fillPrice,
			const char *broker,
			bool isFilled
	);


private:
	const char *m_uri;
	const char *m_DBName;
	const char *m_collectionName;
};


#endif //ARBITO_MONGODBDRIVER_H
