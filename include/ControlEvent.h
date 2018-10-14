
#ifndef ARBITO_CONTROLEVENT_H
#define ARBITO_CONTROLEVENT_H

enum ControlEventType {
	CET_PAUSE = 0,
	CET_RESUME = 1
};

struct ControlEvent {
	ControlEventType type;
	long timestamp_ms;
};

#endif //ARBITO_CONTROLEVENT_H
