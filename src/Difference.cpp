#include <Difference.h>
#include <cstring>

Difference getDifference(const char *difference) {
	if (!strcmp(difference, "A")) {
		return Difference::A;
	}

	if (!strcmp(difference, "B")) {
		return Difference::B;
	}

	return Difference::NONE;
}