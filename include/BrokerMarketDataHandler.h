
#ifndef ARBITO_BROKERMARKETDATAHANDLER_H
#define ARBITO_BROKERMARKETDATAHANDLER_H

#define PUB_BUFFER_SIZE 1024

// Disruptor
#include <Disruptor/IEventHandler.h>

// Aeron
#include <Aeron.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "MarketDataEvent.h"

struct BrokerMarketDataHandler : public Disruptor::IEventHandler<MarketDataEvent> {

	explicit BrokerMarketDataHandler(const std::shared_ptr<aeron::Publication> &messenger_pub);
	void onEvent(MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) override;

private:
	std::shared_ptr<aeron::Publication> m_messenger_pub;
	uint8_t m_buffer[PUB_BUFFER_SIZE];
};

#endif //ARBITO_BROKERMARKETDATAHANDLER_H
