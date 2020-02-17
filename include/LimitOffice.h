
#ifndef ARBITO_LIMITOFFICE_H
#define ARBITO_LIMITOFFICE_H

// Disruptor
#include <Disruptor/Disruptor.h>

// SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

// DOMAIN
#include "FIXSession.h"
#include "PriceEvent.h"

class LimitOffice {
public:
	LimitOffice(
			std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> &prices,
			const char *broker,
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
		m_pricePoller->poll(m_priceHandler);
	}

	void initiate();

	void terminate();

private:
	const char *m_brokerStr;
	enum Broker m_brokerEnum;
	FIXSession m_fixSession;
	struct fix_field *m_limitOrderFields;
	struct fix_message m_limitOrderMessage{};
	struct fix_field *m_replaceOrderFields;
	struct fix_message m_replaceOrderMessage{};
	OnMessageHandler m_onMessageHandler;
	std::shared_ptr<Disruptor::RingBuffer<PriceEvent>> m_prices;
	std::shared_ptr<Disruptor::EventPoller<PriceEvent>> m_pricePoller;
	std::function<bool(PriceEvent &, long, bool)> m_priceHandler;
	double m_lastPrice;
	bool m_orderPlaced;
	bool m_canUpdate;
	std::shared_ptr<spdlog::logger> m_consoleLogger;
	std::shared_ptr<spdlog::logger> m_systemLogger;
};

#endif //ARBITO_LIMITOFFICE_H
