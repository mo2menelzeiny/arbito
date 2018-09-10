
#ifndef ARBITO_REMOTEMARKETDATAEVENT_H
#define ARBITO_REMOTEMARKETDATAEVENT_H

struct RemoteMarketDataEvent {
	double bid;
	double bid_qty;
	double offer;
	double offer_qty;
	long timestamp_us;
	long rec_timestamp_us;
};


#endif //ARBITO_REMOTEMARKETDATAEVENT_H
