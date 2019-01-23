
#ifndef ARBITO_BROKER_H
#define ARBITO_BROKER_H

enum class Broker {
	NONE,
	LMAX,
	IB
};

Broker getBroker(const char *broker);

void getBroker(char *buffer, enum Broker broker);

#endif //ARBITO_BROKER_H
