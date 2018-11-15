
#ifndef ARBITO_CENTRALOFFICE_H
#define ARBITO_CENTRALOFFICE_H

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/NoOpIdleStrategy.h>
#include <FragmentAssembler.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"
#include "sbe/TradeData.h"

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

using namespace std;
using namespace std::chrono;

enum TriggerDifference {
	DIFF_NONE = 0,
	DIFF_A = 1,
	DIFF_B = 2
};

class CentralOffice {
public:
	CentralOffice(
			int publicationPortA,
			const char *publicationHostA,
			int publicationPortB,
			const char *publicationHostB,
			int subscriptionPortA,
			int subscriptionPortB,
			int windowMs,
			int orderDelaySec,
			int maxOrders,
			double diffOpen,
			double diffClose
	);

	void start();

private:
	void work();

private:
	int m_windowMs;
	int m_orderDelaySec;
	int m_maxOrders;
	double m_diffOpen;
	double m_diffClose;
	char m_publicationURIA[64];
	char m_publicationURIB[64];
	char m_subscriptionURIA[64];
	char m_subscriptionURIB[64];
};


#endif //ARBITO_CENTRALOFFICE_H
