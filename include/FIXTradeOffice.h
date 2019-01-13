
#ifndef ARBITO_FIXTRADEOFFICE_H
#define ARBITO_FIXTRADEOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include "FIXSession.h"
#include "MongoDBDriver.h"
#include "BusinessEvent.h"

class FIXTradeOffice {
public:
	FIXTradeOffice(
			std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &inRingBuffer,
			const char *broker,
			double quantity,
			const char *host,
			int port,
			const char *username,
			const char *password,
			const char *sender,
			const char *target,
			int heartbeat,
			bool sslEnabled,
			const char *dbUri,
			const char *dbName
	);

	inline void doWork() {
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
			} catch (std::exception &ex) {
				char message[64];
				sprintf(message, "[%s] Trade Office %s", m_broker, ex.what());
				throw std::runtime_error(message);
			}

			m_consoleLogger->info("[{}] Trade Office OK", m_broker);
			return;
		}

		m_fixSession.poll(m_onMessageHandler);
		m_businessEventPoller->poll(m_businessEventHandler);
	}

	void cleanup();

private:
	std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_inRingBuffer;
	const char *m_broker;
	double m_quantity;
	struct fix_field *m_NOSSFields;
	struct fix_message m_NOSSFixMessage{};
	struct fix_field *m_NOSBFields;
	struct fix_message m_NOSBFixMessage{};
	FIXSession m_fixSession;
	MongoDBDriver m_mongoDriver;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	BrokerEnum m_brokerEnum;
	char m_clOrdIdStrBuff[64];
	char m_orderIdStrBuff[64];
	std::shared_ptr<Disruptor::EventPoller<BusinessEvent>> m_businessEventPoller;
	std::function<bool(BusinessEvent &, long, bool)> m_businessEventHandler;
	OnMessageHandler m_onMessageHandler;
};


#endif //ARBITO_FIXTRADEOFFICE_H
