
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
#include "OrderEvent.h"

class FIXTradeOffice {
public:
	FIXTradeOffice(
			std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &inRingBuffer,
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
		m_orderEventPoller->poll(m_orderEventHandler);
	}

	void initiate();

	void terminate();

private:
	std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> m_inRingBuffer;
	const char *m_brokerStr;
	double m_quantity;
	struct fix_field *m_limitOrderFields;
	struct fix_message m_limitOrderMessage{};
	struct fix_field *m_marketOrderFields;
	struct fix_message m_marketOrderMessage{};
	FIXSession m_fixSession;
	MongoDBDriver m_mongoDriver;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
	enum Broker m_brokerEnum;
	char m_clOrdIdStrBuff[64];
	char m_orderIdStrBuff[64];
	std::shared_ptr<Disruptor::EventPoller<OrderEvent>> m_orderEventPoller;
	std::function<bool(OrderEvent &, long, bool)> m_orderEventHandler;
	OnMessageHandler m_onMessageHandler;
};


#endif //ARBITO_FIXTRADEOFFICE_H
