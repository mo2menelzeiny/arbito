
#ifndef ARBITO_SYSTEMEVENT_H
#define ARBITO_SYSTEMEVENT_H

struct SystemEvent {
	char message[64];
	SystemEventType type;
};

#endif //ARBITO_SYSTEMEVENT_H
