
#ifndef ARBITO_REMOTEMARKETDATAEVENT_H
#define ARBITO_REMOTEMARKETDATAEVENT_H

struct RemoteMarketDataEvent {
	double bid;
	double offer;
	long timestamp_ms;
	long rec_timestamp_ms;
};
// 16 bytes for doubles + 8 bytes for longs = 24 bytes

#endif //ARBITO_REMOTEMARKETDATAEVENT_H
