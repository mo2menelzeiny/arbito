
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
		m_fixSession.poll(m_onMessageHandler);
		m_businessEventPoller->poll(m_businessEventHandler);
	}

	void initiate();

	void terminate();

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
