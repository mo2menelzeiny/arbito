
#ifndef ARBITO_ARBITRAGEDATAEVENT_H
#define ARBITO_ARBITRAGEDATAEVENT_H

#include "MarketDataEvent.h"

struct ArbitrageDataEvent {
	MarketDataEvent l1, l2;
	const char *timestamp;

	double offer1_minus_bid2() {
		return l1.offer - l2.bid;
	};

	double offer2_minus_bid1() {
		return l2.offer - l1.bid;
	}

};

#endif //ARBITO_ARBITRAGEDATAEVENT_H
