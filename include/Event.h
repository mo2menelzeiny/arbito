
#ifndef ARBITO_EVENT_H
#define ARBITO_EVENT_H
enum EventType {
	MARKET_DATA = 0,
	REJECT = 1,
	CONFIRM = 2
};

enum Broker {
	BROKER_LMAX = 0,
	BROKER_SWISSQUOTE = 1
};

struct Event {
	enum EventType event_type;
	enum Broker broker;
	double bid;
	double bid_qty;
	double offer;
	double offer_qty;
};


#endif //ARBITO_EVENT_H
