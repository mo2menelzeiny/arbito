
#include "BrokerMarketDataHandler.h"

BrokerMarketDataHandler::BrokerMarketDataHandler(const std::shared_ptr<aeron::Publication> &messenger_pub)
		: m_messenger_pub(messenger_pub) {
	m_atomic_buffer.wrap(m_buffer, PUB_BUFFER_SIZE);
}

void BrokerMarketDataHandler::onEvent(MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) {
	m_msg_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, PUB_BUFFER_SIZE)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	m_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer), sbe::MessageHeader::encodedLength(), PUB_BUFFER_SIZE)
			.bid(data.bid)
			.bidQty(data.bid_qty)
			.offer(data.offer)
			.offerQty(data.offer_qty);

	aeron::index_t len = sbe::MessageHeader::encodedLength() + m_market_data.encodedLength();
	std::int64_t result;
	do {
		result = m_messenger_pub->offer(m_atomic_buffer, 0, len);
	} while (result < -1);
}
