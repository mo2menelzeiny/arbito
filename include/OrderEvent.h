
#ifndef ARBITO_ORDEREVENT_H
#define ARBITO_ORDEREVENT_H

#include "Broker.h"
#include "OrderType.h"
#include "OrderSide.h"

struct OrderEvent {
	enum Broker broker;
	enum OrderType order;
	enum OrderSide side;
	double price;
	unsigned long id;
};

#endif //ARBITO_ORDEREVENT_H
