
#ifndef ARBITO_FIXMARKETOFFICE_H
#define ARBITO_FIXMARKETOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "FIXSession.h"
#include "PriceEvent.h"

class FIXMarketOffice {
public:
	FIXMarketOffice(
			std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &outRingBuffer,
			const char *broker,
			double quantity,
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat,
			bool sslEnabled
	);

	inline void doWork() {
		m_fixSession.poll(m_onMessageHandler);
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> m_outRingBuffer;
	const char *m_brokerStr;
	enum Broker m_brokerEnum;
	double m_quantity;
	struct fix_field *m_MDRFixFields[4]{};
	struct fix_message m_MDRFixMessages[4]{};
	FIXSession m_fixSession;
	OnMessageHandler m_onMessageHandler;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	long m_sequence;
	double m_bid;
	double m_bidQty;
	double m_ask;
	double m_askQty;
};


#endif //ARBITO_FIXMARKETOFFICE_H
