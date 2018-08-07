
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

struct MarketDataEvent {
	double bid;
	double bid_qty;
	double offer;
	double offer_qty;
	const char *timestamp;
};


#endif //ARBITO_MARKETDATAEVENT_H
