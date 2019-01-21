
#ifndef ARBITO_MARKETEVENT_H
#define ARBITO_MARKETEVENT_H

#include "BrokerEnum.h"

struct MarketEvent {
	double bid;
	double offer;
	long sequence;
	BrokerEnum broker;
};


#endif //ARBITO_MARKETEVENT_H
