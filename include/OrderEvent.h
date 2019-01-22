
#ifndef ARBITO_ORDEREVENT_H
#define ARBITO_ORDEREVENT_H

#include "BrokerEnum.h"

struct OrderEvent {
	BrokerEnum buy;
	BrokerEnum sell;
	unsigned long id;
};

#endif //ARBITO_ORDEREVENT_H
