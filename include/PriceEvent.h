
#ifndef ARBITO_PRICEEVENT_H
#define ARBITO_PRICEEVENT_H

#include "Broker.h"

struct PriceEvent {
	enum Broker broker;
	double bid;
	double offer;
	long sequence;
};


#endif //ARBITO_PRICEEVENT_H
