
#ifndef ARBITO_CONTROLEVENT_H
#define ARBITO_CONTROLEVENT_H

enum ControlEventSource {
	CES_TRADE_OFFICE = 0,
	CES_MARKET_OFFICE = 1,
	CES_BUSINESS_OFFICE = 2,
	CES_REMOTE_M_OFFICE = 3,
	CES_RECORDS_OFFICE = 4,
	CES_EXCLUSIVE_M_OFFICE = 5,
	CES_MAIN = 6
};

enum ControlEventType {
	CET_PAUSE = 0,
	CET_RESUME = 1
};

struct ControlEvent {
	ControlEventSource source;
	ControlEventType type;
	long timestamp_us;
};

#endif //ARBITO_CONTROLEVENT_H
