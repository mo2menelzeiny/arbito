
#include <FIXMarketOffice.h>

FIXMarketOffice::FIXMarketOffice(
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
) : m_outRingBuffer(outRingBuffer),
    m_brokerStr(broker),
    m_brokerEnum(getBroker(broker)),
    m_quantity(quantity),
    m_fixSession(
		    host,
		    port,
		    username,
		    password,
		    sender,
		    target,
		    heartbeat,
		    sslEnabled
    ),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_sequence(0),
    m_offerIdx(m_brokerEnum == Broker::LMAX ? 4 : 2),
    m_offerQtyIdx(m_offerIdx - 1) {

	if (m_brokerEnum == Broker::LMAX) {
		struct fix_field fields[] = {
				FIX_STRING_FIELD(MDReqID, "MARKET-DATA-REQUEST"),
				FIX_CHAR_FIELD(SubscriptionRequestType, '1'),
				FIX_INT_FIELD(MarketDepth, 1),
				FIX_INT_FIELD(MDUpdateType, 0),
				FIX_INT_FIELD(NoMDEntryTypes, 2),
				FIX_CHAR_FIELD(MDEntryType, '0'),
				FIX_CHAR_FIELD(MDEntryType, '1'),
				FIX_INT_FIELD(NoRelatedSym, 1),
				FIX_STRING_FIELD(SecurityID, "4001"),
				FIX_STRING_FIELD(SecurityIDSource, "8")
		};
		unsigned long size = ARRAY_SIZE(fields);
		m_MDRFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_MDRFields, fields, size * sizeof(fix_field));
		m_MDRFixMessage.nr_fields = size;
	}

	m_MDRFixMessage.type = FIX_MSG_TYPE_MARKET_DATA_REQUEST;
	m_MDRFixMessage.fields = m_MDRFields;

	m_onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {
				double bid = fix_get_float(msg, MDEntryPx, 0);
				double bidQty = fix_get_float(msg, MDEntrySize, 0);
				double offer = fix_get_field_at(msg, static_cast<int>(msg->nr_fields - m_offerIdx))->float_value;
				double offerQty = fix_get_field_at(msg, static_cast<int>(msg->nr_fields - m_offerQtyIdx))->float_value;

				if (m_quantity > offerQty || m_quantity > bidQty) {
					break;
				}

				++m_sequence;

				auto nextSequence = m_outRingBuffer->next();
				(*m_outRingBuffer)[nextSequence] = {m_brokerEnum, bid, offer, m_sequence};
				m_outRingBuffer->publish(nextSequence);

				m_systemLogger->info("[{}][{}] bid: {} offer: {}", m_brokerStr, m_sequence, bid, offer);
			}
				break;

			default:
				char text[512];
				msg_string(text, msg, 512);
				m_consoleLogger->error("[{}] Market Office Unhandled FAILED {}", m_brokerStr, text);
				break;
		}
	});
}

void FIXMarketOffice::initiate() {
	m_fixSession.initiate();
	m_fixSession.send(&m_MDRFixMessage);
	m_consoleLogger->info("[{}] Market Office OK", m_brokerStr);
}

void FIXMarketOffice::terminate() {
	m_fixSession.terminate();
}

