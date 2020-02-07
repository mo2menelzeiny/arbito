#include <Broker.h>
#include <cstring>
#include <cstdio>

Broker getBroker(const char *broker) {
	if(!strcmp(broker, "IB")) {
		return Broker::IB;
	}

	if(!strcmp(broker, "LMAX")) {
		return Broker::LMAX;
	}

	if(!strcmp(broker, "FASTMATCH")) {
		return Broker::FASTMATCH;
	}

	return Broker::NONE;
}

void getBroker(char *buffer, enum Broker broker) {
	switch (broker) {
		case Broker::NONE:
			sprintf(buffer, "NONE");
			break;
		case Broker::LMAX:
			sprintf(buffer, "LMAX");
			break;
		case Broker::IB:
			sprintf(buffer, "IB");
			break;
		case Broker::FASTMATCH:
			sprintf(buffer, "FASTMATCH");
	}
}