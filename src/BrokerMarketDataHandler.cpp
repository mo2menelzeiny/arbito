
#include "BrokerMarketDataHandler.h"

BrokerMarketDataHandler::BrokerMarketDataHandler(const std::shared_ptr<aeron::Publication> &messenger_pub)
		: m_messenger_pub(messenger_pub) {
}

void BrokerMarketDataHandler::onEvent(MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) {
	sbe::MessageHeader m_msg_header;
	m_msg_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, PUB_BUFFER_SIZE)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	sbe::MarketData m_market_data;
	m_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer), m_msg_header.encodedLength(), PUB_BUFFER_SIZE)
			.bid(data.bid)
			.bidQty(data.bid_qty)
			.offer(data.offer)
			.offerQty(data.offer_qty);

	aeron::index_t len = m_msg_header.encodedLength() + m_market_data.encodedLength();
	aeron::concurrent::AtomicBuffer srcBuffer(m_buffer, PUB_BUFFER_SIZE);
	srcBuffer.putBytes(0, reinterpret_cast<const uint8_t *>(m_buffer), len);
	std::int64_t result;
	do {
		result = m_messenger_pub->offer(srcBuffer, 0, len);
	} while (result < 0UL);
}
