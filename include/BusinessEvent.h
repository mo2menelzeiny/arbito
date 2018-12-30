
#ifndef ARBITO_BUSINESSEVENT_H
#define ARBITO_BUSINESSEVENT_H

#include "Broker.h"

struct BusinessEvent {
	Broker buy;
	Broker sell;
	unsigned long id;
};

#endif //ARBITO_BUSINESSEVENT_H
