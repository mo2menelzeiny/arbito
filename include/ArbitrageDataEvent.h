
#ifndef ARBITO_ARBITRAGEDATAEVENT_H
#define ARBITO_ARBITRAGEDATAEVENT_H

#include "MarketDataEvent.h"

struct ArbitrageDataEvent {
	MarketDataEvent l1, l2;
	const char *timestamp;

	double bid1_minus_offer2() {
		return l1.bid - l2.offer;
	};

	double bid2_minus_offer1() {
		return l2.bid - l1.offer;
	}

};

#endif //ARBITO_ARBITRAGEDATAEVENT_H
