#include "LimitOffice.h"

LimitOffice::LimitOffice(
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
) : m_prices(prices),
    m_brokerStr(broker),
    m_brokerEnum(getBroker(broker)),
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
    m_pricePoller(prices->newPoller()),
    m_orderPlaced(false),
    m_canUpdate(false),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")) {

	// limit order template
	struct fix_field limitOrderFields[] = {
			FIX_STRING_FIELD(ClOrdID, "LIMIT-BUY"),
			FIX_STRING_FIELD(Symbol, "EUR/USD"),
			FIX_CHAR_FIELD(Side, '1'),
			FIX_STRING_FIELD(TransactTime, ""),
			FIX_FLOAT_FIELD(OrderQty, 100000),
			FIX_CHAR_FIELD(OrdType, '2'),
			FIX_CHAR_FIELD(TimeInForce, '0'), // DAY
			FIX_FLOAT_FIELD(Price, 0)
	};

	unsigned long size = ARRAY_SIZE(limitOrderFields);
	m_limitOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
	memcpy(m_limitOrderFields, limitOrderFields, size * sizeof(fix_field));
	m_limitOrderMessage.nr_fields = size;

	m_limitOrderMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_limitOrderMessage.fields = m_limitOrderFields;

	// replace order template
	struct fix_field replaceOrderFields[] = {
			FIX_STRING_FIELD(Symbol, "EUR/USD"),
			FIX_STRING_FIELD(OrigClOrdID, "LIMIT-BUY"),
			FIX_STRING_FIELD(ClOrdID, "LIMIT-BUY"),
			FIX_CHAR_FIELD(Side, '1'),
			FIX_STRING_FIELD(TransactTime, ""),
			FIX_FLOAT_FIELD(OrderQty, 100000),
			FIX_CHAR_FIELD(OrdType, '2'),
			FIX_CHAR_FIELD(TimeInForce, '0'), // DAY
			FIX_FLOAT_FIELD(Price, 0)
	};

	size = ARRAY_SIZE(replaceOrderFields);
	m_replaceOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
	memcpy(m_replaceOrderFields, replaceOrderFields, size * sizeof(fix_field));
	m_replaceOrderMessage.nr_fields = size;

	m_replaceOrderMessage.type = FIX_MSG_TYPE_ORDER_CANCEL_REPLACE_REQUEST;
	m_replaceOrderMessage.fields = m_replaceOrderFields;

	// fix session message handler
	m_onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		char text[512];
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT: {
				// restart when order is filled
				char execType = fix_get_field(msg, ExecType)->string_value[0];
				if (execType == '2') {
					m_orderPlaced = false;
					m_canUpdate = false;
				}

				if (execType == '0' || execType == '5') {
					m_canUpdate = true;
				}

				msg_string(text, msg, 512);
				m_systemLogger->info("[{}] Limit Office {}", m_brokerStr, text);
			}
				break;
			case FIX_MSG_TYPE_ORDER_CANCEL_REJECT:
				msg_string(text, msg, 512);
				m_systemLogger->info("[{}] Limit Office {}", m_brokerStr, text);
				break;
			default:
				msg_string(text, msg, 512);
				m_consoleLogger->error("[{}] Limit Office {}", m_brokerStr, text);
				break;
		}
	});

	// price events handler
	m_priceHandler = [&](PriceEvent &event, int64_t seq, bool endOfBatch) -> bool {
		// place first order only once
		if (!m_orderPlaced && event.sequence > 10) {
			fix_get_field(&m_limitOrderMessage, Price)->float_value = event.bid + 0.00001;
			fix_get_field(&m_limitOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_limitOrderMessage);
			m_orderPlaced = true;
			m_lastPrice = event.bid;
			m_systemLogger->info("[{}] {} {} {} {}", m_brokerStr,
			                     "CURRENT BID:", event.bid,
			                     "BUY ON", event.bid + 0.00001);
		}

		// frequently update
		if (m_canUpdate && m_lastPrice != event.bid) {
			fix_get_field(&m_replaceOrderMessage, Price)->float_value = event.bid + 0.00001;
			fix_get_field(&m_replaceOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_replaceOrderMessage);
			m_lastPrice = event.bid;
			m_canUpdate = false;
			m_systemLogger->info("[{}] {} {} {} {}", m_brokerStr,
			                     "CURRENT BID:", event.bid,
			                     "UPDATE ON", event.bid + 0.00001);
		}

		return true;
	};

}

void LimitOffice::initiate() {
	m_fixSession.initiate();
	m_prices->addGatingSequences({m_pricePoller->sequence()});
	m_consoleLogger->info("[{}] Limit Office OK", m_brokerStr);
}

void LimitOffice::terminate() {
	m_fixSession.terminate();
	m_prices->removeGatingSequence(m_pricePoller->sequence());
}
