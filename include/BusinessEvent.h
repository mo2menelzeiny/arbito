
#ifndef ARBITO_BUSINESSEVENT_H
#define ARBITO_BUSINESSEVENT_H

#include "BrokerEnum.h"

struct BusinessEvent {
	BrokerEnum buy;
	BrokerEnum sell;
	unsigned long id;
};

#endif //ARBITO_BUSINESSEVENT_H
