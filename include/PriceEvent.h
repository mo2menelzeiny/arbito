
#ifndef ARBITO_PRICEEVENT_H
#define ARBITO_PRICEEVENT_H

#include "BrokerEnum.h"

struct PriceEvent {
	double bid;
	double offer;
	long sequence;
	BrokerEnum broker;
};


#endif //ARBITO_PRICEEVENT_H
