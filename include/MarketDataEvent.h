
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

#include "Broker.h"

struct MarketDataEvent {
	double bid;
	double offer;
	long sequence;
	Broker broker;
};


#endif //ARBITO_MARKETDATAEVENT_H
