
#ifndef ARBITO_ARBITRAGEDATAEVENT_H
#define ARBITO_ARBITRAGEDATAEVENT_H

#include "MarketDataEvent.h"

struct ArbitrageDataEvent {
	MarketDataEvent local, remote;

	double bid1_minus_offer2() {
		return local.bid - remote.offer;
	};

	double bid2_minus_offer1() {
		return remote.bid - local.offer;
	}

};

#endif //ARBITO_ARBITRAGEDATAEVENT_H
