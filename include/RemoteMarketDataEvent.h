
#ifndef ARBITO_REMOTEMARKETDATAEVENT_H
#define ARBITO_REMOTEMARKETDATAEVENT_H

struct RemoteMarketDataEvent {
	double bid;
	double offer;
	long timestamp_us;
	long rec_timestamp_us;
};


#endif //ARBITO_REMOTEMARKETDATAEVENT_H
