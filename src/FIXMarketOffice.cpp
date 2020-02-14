
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
    m_mid(0),
    m_bid(-99),
    m_bidQty(0),
    m_ask(99),
    m_offerQty(0) {

	if (m_brokerEnum == Broker::LMAX) {
		int mdr_idx;
		unsigned long size;

		mdr_idx = 0;
		struct fix_field fields0[] = {
				FIX_STRING_FIELD(MDReqID, "MDR-0"),
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
		size = ARRAY_SIZE(fields0);
		m_MDRFixFields[mdr_idx] = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_MDRFixFields[mdr_idx], fields0, size * sizeof(fix_field));
		m_MDRFixMessages[mdr_idx].nr_fields = size;
		m_MDRFixMessages[mdr_idx].type = FIX_MSG_TYPE_MARKET_DATA_REQUEST;
		m_MDRFixMessages[mdr_idx].fields = m_MDRFixFields[mdr_idx];

	} else if (m_brokerEnum == Broker::FASTMATCH) {

		int mdr_idx;
		unsigned long size;

		mdr_idx = 0;
		struct fix_field fields0[] = {
				FIX_STRING_FIELD(MDReqID, "MDR-0"),
				FIX_CHAR_FIELD(SubscriptionRequestType, '1'),
				FIX_INT_FIELD(MarketDepth, 1),
				FIX_INT_FIELD(MDUpdateType, 1),
				FIX_CHAR_FIELD(AggregatedBook, 'N'),
				FIX_INT_FIELD(NoMDEntryTypes, 3),
				FIX_CHAR_FIELD(MDEntryType, '0'),
				FIX_CHAR_FIELD(MDEntryType, '1'),
				FIX_CHAR_FIELD(MDEntryType, 'H'),
				FIX_INT_FIELD(NoRelatedSym, 1),
				FIX_STRING_FIELD(Symbol, "EUR/USD")
		};
		size = ARRAY_SIZE(fields0);
		m_MDRFixFields[mdr_idx] = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_MDRFixFields[mdr_idx], fields0, size * sizeof(fix_field));
		m_MDRFixMessages[mdr_idx].nr_fields = size;
		m_MDRFixMessages[mdr_idx].type = FIX_MSG_TYPE_MARKET_DATA_REQUEST;
		m_MDRFixMessages[mdr_idx].fields = m_MDRFixFields[mdr_idx];
	}

	m_onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH: {

				for (int i = 0; i < msg->nr_fields; i++) {
					if (msg->fields[i].tag == MDEntryType) {
						if (msg->fields[i].string_value[0] == '0') { // BID
							m_bid = msg->fields[i + 2].float_value;
							m_bidQty = msg->fields[i + 3].float_value;
						} else if (msg->fields[i].string_value[0] == '1') { // ASK
							m_ask = msg->fields[i + 2].float_value;
							m_offerQty = msg->fields[i + 3].float_value;
						} else if (msg->fields[i].string_value[0] == 'H') { // MID
							m_mid = msg->fields[i + 2].float_value;
						}
					}
				}

				++m_sequence;

				auto nextSequence = m_outRingBuffer->next();
				(*m_outRingBuffer)[nextSequence] = {m_brokerEnum, m_bid, m_ask, m_mid, m_sequence};
				m_outRingBuffer->publish(nextSequence);

				m_systemLogger->info("[{}][{}] bid: {} ask: {} mid: {}",
						m_brokerStr, m_sequence, m_bid, m_ask, m_mid);

				/*char text[512];
				msg_string(text, msg, 512);
				m_systemLogger->info("[{}][{}] {}", m_brokerStr, m_sequence, text);*/
			}
				break;

			case FIX_MSG_TYPE_MARKET_DATA_INCREMENTAL_REFRESH: {

				for (int i = 0; i < msg->nr_fields; i++) {
					if (msg->fields[i].tag == MDUpdateAction && msg->fields[i].string_value[0] == '0') { // NEW
						if (msg->fields[i + 2].string_value[0] == '0') { // BID
							m_bid = msg->fields[i + 4].float_value;
							m_bidQty = msg->fields[i + 5].float_value;
						} else if (msg->fields[i + 2].string_value[0] == '1') { // ASK
							m_ask = msg->fields[i + 4].float_value;
							m_offerQty = msg->fields[i + 5].float_value;
						} else if (msg->fields[i + 2].string_value[0] == 'H') { // MID
							m_mid = msg->fields[i + 4].float_value;
						}
					}
				}

				++m_sequence;

				auto nextSequence = m_outRingBuffer->next();
				(*m_outRingBuffer)[nextSequence] = {m_brokerEnum, m_bid, m_ask, m_mid, m_sequence};
				m_outRingBuffer->publish(nextSequence);

				m_systemLogger->info("[{}][{}] bid: {} ask: {} mid: {}",
				                     m_brokerStr, m_sequence, m_bid, m_ask, m_mid);

				/*char text[512];
				msg_string(text, msg, 512);
				m_systemLogger->info("[{}][{}] {}", m_brokerStr, m_sequence, text);*/
			}
				break;

			default:
				char text[512];
				msg_string(text, msg, 512);
				m_consoleLogger->error("[{}] Market Office FAILED {}", m_brokerStr, text);
				break;
		}
	});
}

void FIXMarketOffice::initiate() {
	m_fixSession.initiate();

	for (auto mdrMsg : m_MDRFixMessages) {
		if (mdrMsg.type == FIX_MSG_TYPE_MARKET_DATA_REQUEST) {
			m_fixSession.send(&mdrMsg);
		}
	}

	m_consoleLogger->info("[{}] Market Office OK", m_brokerStr);
}

void FIXMarketOffice::terminate() {
	m_fixSession.terminate();
}

