
#ifndef ARBITO_ORDEREVENT_H
#define ARBITO_ORDEREVENT_H

struct TradeEvent {
	char orderId[64];
	char clOrdId[64];
	char side;
	double avgPx;
	long timestamp_us;
};

#endif //ARBITO_ORDEREVENT_H
