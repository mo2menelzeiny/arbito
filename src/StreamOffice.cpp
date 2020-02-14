
#include "StreamOffice.h"

StreamOffice::StreamOffice(PriceRingBuffer &priceRingBuffer)
		: m_priceRingBuffer(priceRingBuffer),
		  m_natsSession(),
		  m_priceEventPoller(m_priceRingBuffer->newPoller()),
		  m_consoleLogger(spdlog::get("console")),
		  m_systemLogger(spdlog::get("system")) {
}

StreamOffice::StreamOffice(
		PriceRingBuffer &priceRingBuffer,
		const char *host,
		const char *username,
		const char *password
) : m_priceRingBuffer(priceRingBuffer),
    m_natsSession(
		    host,
		    username,
		    password
    ),
    m_priceEventPoller(m_priceRingBuffer->newPoller()),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")) {

}

void StreamOffice::initiate() {
	m_priceEventHandler = [&](PriceEvent &event, int64_t seq, bool endOfBatch) -> bool {
		std::string channel = "arbito.prices";
		switch (event.broker) {
			case Broker::LMAX:
				channel += ".LMAX";
				break;
			case Broker::IB:
				channel += ".IB";
				break;
			case Broker::FASTMATCH:
				channel += ".FASTMATCH";
				break;
			default:
				break;
		}

		std::string json = "{\"bid\":" + std::to_string(event.bid)
		                   + ", \"ask\":" + std::to_string(event.ask)
		                   + ", \"mid\":" + std::to_string(event.mid)
		                   + "}";

		m_natsSession.publishString(natsConnection_PublishString, channel.c_str(), json.c_str());

		return true;
	};

	m_priceRingBuffer->addGatingSequences({m_priceEventPoller->sequence()});

	m_natsSession.initiate(
			natsOptions_Create,
			natsOptions_SetSendAsap,
			natsOptions_SetURL,
			natsConnection_Connect
	);

	m_consoleLogger->info("Stream Office OK");
}

void StreamOffice::terminate() {
	m_natsSession.terminate();
	m_priceRingBuffer->removeGatingSequence({m_priceEventPoller->sequence()});
}
