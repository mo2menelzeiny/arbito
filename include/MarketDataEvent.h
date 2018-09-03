
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

struct MarketDataEvent {
	double bid;
	double bid_qty;
	double offer;
	double offer_qty;
	__syscall_slong_t timestamp_ns;
};


#endif //ARBITO_MARKETDATAEVENT_H
