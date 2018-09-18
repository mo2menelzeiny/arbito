
#ifndef ARBITO_BUSINESSEVENT_H
#define ARBITO_BUSINESSEVENT_H

#include "OpenSide.h"

struct BusinessEvent {
	char side;
	int clOrdId;
	double trigger_px;
	double remote_px;
	long timestamp_us;
	OpenSide open_side;
	int orders_count;
};

#endif //ARBITO_BUSINESSEVENT_H
