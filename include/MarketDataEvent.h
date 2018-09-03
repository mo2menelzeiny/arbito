
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

struct MarketDataEvent {
	double bid;
	double bid_qty;
	double offer;
	double offer_qty;
	struct timespec timestamp_ns;
};


#endif //ARBITO_MARKETDATAEVENT_H
