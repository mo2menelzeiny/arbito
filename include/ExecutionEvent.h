
#ifndef ARBITO_EXECUTIONEVENT_H
#define ARBITO_EXECUTIONEVENT_H

#include "Broker.h"

struct ExecutionEvent {
	enum Broker broker;
	enum OrderSide side;
	bool isFilled;
};

#endif //ARBITO_EXECUTIONEVENT_H
