
#ifndef ARBITO_MARKETDATAEVENT_H
#define ARBITO_MARKETDATAEVENT_H

struct MarketDataEvent {
	double bid;
	double offer;
	long timestamp_ms;
	char sending_time[64];
};


#endif //ARBITO_MARKETDATAEVENT_H
