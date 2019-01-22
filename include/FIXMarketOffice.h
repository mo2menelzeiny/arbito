
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
	const char *m_broker;
	double m_quantity;
	struct fix_field *m_MDRFields;
	struct fix_message m_MDRFixMessage{};
	FIXSession m_fixSession;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	long m_sequence;
	OnMessageHandler m_onMessageHandler;
	unsigned long m_offerIdx;
	unsigned long m_offerQtyIdx;
	BrokerEnum m_brokerEnum;
};


#endif //ARBITO_FIXMARKETOFFICE_H
