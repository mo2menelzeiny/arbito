
#include "BusinessOffice.h"

BusinessOffice::BusinessOffice(const double spread, const double diff_open, const double diff_close)
		: m_spread(spread), m_diff_open(diff_open), m_diff_close(diff_close) {
}

void BusinessOffice::onEvent(Event &data, std::int64_t sequence, bool endOfBatch) {
	// TODO: Implement Filter Logic
	// TODO: Implement Business Logic
}
