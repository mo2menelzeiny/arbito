
#ifndef ARBITO_DIFFERENCE_H
#define ARBITO_DIFFERENCE_H

enum class Difference {
	NONE = 0,
	A = 1,
	B = 2
};

Difference getDifference(const char *difference);

#endif //ARBITO_DIFFERENCE_H
