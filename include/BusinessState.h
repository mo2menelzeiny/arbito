
#ifndef ARBITO_RECOVERYDATA_H
#define ARBITO_RECOVERYDATA_H

enum OpenSide {
	NONE = 0,
	OPEN_BUY = 1,
	OPEN_SELL = 2
};

struct BusinessState {
	OpenSide open_side;
	int orders_count;
};

#endif //ARBITO_RECOVERYDATA_H
