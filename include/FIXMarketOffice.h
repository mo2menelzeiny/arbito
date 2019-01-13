
#ifndef ARBITO_FIXMARKETOFFICE_H
#define ARBITO_FIXMARKETOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "FIXSession.h"
#include "MarketDataEvent.h"

class FIXMarketOffice {
public:
	FIXMarketOffice(
			std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &outRingBuffer,
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
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
			} catch (std::exception &ex) {
				char message[64];
				sprintf(message, "[%s] Market Office %s", m_broker, ex.what());
				throw std::runtime_error(message);
			}

			m_fixSession.send(&m_MDRFixMessage);
			m_consoleLogger->info("[{}] Market Office OK", m_broker);
			return;
		}

		m_fixSession.poll(m_onMessageHandler);
	}

	void cleanup();

private:
	std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_outRingBuffer;
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
};


#endif //ARBITO_FIXMARKETOFFICE_H
