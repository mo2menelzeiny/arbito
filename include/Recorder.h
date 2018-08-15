
#ifndef ARBITO_RECORDER_H
#define ARBITO_RECORDER_H

// MongoDB
#include <mongoc.h>

// C
#include <chrono>

// Domain
#include "ArbitrageDataEvent.h"

enum SystemRecordType {
	SYSTEM_RECORD_TYPE_ERROR = 0,
	SYSTEM_RECORD_TYPE_SUCCESS = 1,
};

enum OrderRecordType {
	ORDER_RECORD_TYPE_BUY = 0,
	ORDER_RECORD_TYPE_SELL = 1
};

enum OrderRecordState {
	ORDER_RECORD_STATE_OPEN = 0,
	ORDER_RECORD_STATE_CLOSE = 1,
	ORDER_RECORD_STATE_INIT = 2

};

enum OrderTriggerType {
	ORDER_TRIGGER_TYPE_CURRENT_DIFF_2 = 0,
	ORDER_TRIGGER_TYPE_CURRENT_DIFF_1 = 1,
};

class Recorder {
public:
	Recorder(const char *uri_string, int broker_num, const char *db_name);

	void recordSystem(const char *message, SystemRecordType type);

	void recordArbitrage(ArbitrageDataEvent &event);

	void recordOrder(double broker_price, double trigger_price, OrderRecordType order_type, double trigger_diff,
		                 OrderTriggerType trigger_type, OrderRecordState order_state);

private:
	int ping();

private:
	mongoc_uri_t *m_uri;
	mongoc_client_pool_t *m_pool;
	const char *m_broker_name;
	const char *m_db_name;
};


#endif //ARBITO_RECORDER_H
