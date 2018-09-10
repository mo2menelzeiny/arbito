
#ifndef ARBITO_BUSINESSEVENT_H
#define ARBITO_BUSINESSEVENT_H

struct BusinessEvent {
	char side;
	int clOrdId;
	double trigger_px;
	long timestamp_us;
};

#endif //ARBITO_BUSINESSEVENT_H
