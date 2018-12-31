
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

#include "BrokerEnum.h"

struct MarketDataEvent {
	double bid;
	double offer;
	long sequence;
	BrokerEnum broker;
};


#endif //ARBITO_MARKETDATAEVENT_H
