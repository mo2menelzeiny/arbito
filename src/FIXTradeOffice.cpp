
#include <FIXTradeOffice.h>

FIXTradeOffice::FIXTradeOffice(
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
) : m_inRingBuffer(inRingBuffer),
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
    m_mongoDriver(dbUri, dbName, "coll_orders"),
    m_consoleLogger(spdlog::get("console")),
    m_systemLogger(spdlog::get("system")),
    m_orderEventPoller(m_inRingBuffer->newPoller()) {
	// NOSB = New Single Order Buy
	// NOSS = New Single Order Sell
	if (!strcmp(broker, "LMAX")) {
		struct fix_field NOSSFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-SELL"),
				FIX_STRING_FIELD(SecurityID, "4001"),
				FIX_STRING_FIELD(SecurityIDSource, "8"),
				FIX_CHAR_FIELD(Side, '2'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_CHAR_FIELD(TimeInForce, '4'),
				FIX_FLOAT_FIELD(Price, 0)
		};
		unsigned long size = ARRAY_SIZE(NOSSFields);
		m_NOSSFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSSFields, NOSSFields, size * sizeof(fix_field));
		m_NOSSFixMessage.nr_fields = size;

		struct fix_field NOSBFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(SecurityID, "4001"),
				FIX_STRING_FIELD(SecurityIDSource, "8"),
				FIX_CHAR_FIELD(Side, '1'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_CHAR_FIELD(TimeInForce, '4'),
				FIX_FLOAT_FIELD(Price, 0)
		};

		size = ARRAY_SIZE(NOSBFields);
		m_NOSBFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSBFields, NOSBFields, size * sizeof(fix_field));
		m_NOSBFixMessage.nr_fields = size;
	}

	if (!strcmp(broker, "IB")) {
		struct fix_field NOSSFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-SELL"),
				FIX_STRING_FIELD(Symbol, "EUR"),
				FIX_STRING_FIELD(Currency, "USD"),
				FIX_STRING_FIELD(SecurityType, "CFD"),
				FIX_CHAR_FIELD(Side, '2'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_STRING_FIELD(Account, "U2707646"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "SMART"),
				FIX_CHAR_FIELD(TimeInForce, '3'),
				FIX_FLOAT_FIELD(Price, 0)
		};
		unsigned long size = ARRAY_SIZE(NOSSFields);
		m_NOSSFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSSFields, NOSSFields, size * sizeof(fix_field));
		m_NOSSFixMessage.nr_fields = size;

		struct fix_field NOSBFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(Symbol, "EUR"),
				FIX_STRING_FIELD(Currency, "USD"),
				FIX_STRING_FIELD(SecurityType, "CFD"),
				FIX_CHAR_FIELD(Side, '1'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_STRING_FIELD(Account, "U2707646"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "SMART"),
				FIX_CHAR_FIELD(TimeInForce, '3'),
				FIX_FLOAT_FIELD(Price, 0)
		};

		size = ARRAY_SIZE(NOSBFields);
		m_NOSBFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSBFields, NOSBFields, size * sizeof(fix_field));
		m_NOSBFixMessage.nr_fields = size;
	}

	m_NOSSFixMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_NOSSFixMessage.fields = m_NOSSFields;

	m_NOSBFixMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_NOSBFixMessage.fields = m_NOSBFields;

	m_orderEventHandler = [&](OrderEvent &event, int64_t seq, bool endOfBatch) -> bool {
		if(event.broker != m_brokerEnum) {
			return false;
		}

		sprintf(m_clOrdIdStrBuff, "%lu", event.id);

		if (event.order == OrderType::LIMIT && event.side == OrderSide::BUY) {

		}

		if (event.order == OrderType::LIMIT && event.side == OrderSide::SELL) {

		}

		if (event.order == OrderType::MARKET && event.side == OrderSide::BUY) {
			fix_get_field(&m_NOSBFixMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_NOSBFixMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_NOSBFixMessage);
		}

		if (event.order == OrderType::MARKET && event.side == OrderSide::SELL) {
			fix_get_field(&m_NOSSFixMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_NOSSFixMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_NOSSFixMessage);
		}

		m_systemLogger->info("[{}] {} id: {}", m_brokerStr, event.side == OrderSide::BUY ? "BUY" : "SELL", m_clOrdIdStrBuff);

		return false;
	};

	m_onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT: {
				char execType = fix_get_field(msg, ExecType)->string_value[0];
				char ordStatus = fix_get_field(msg, OrdStatus)->string_value[0];

				if ((execType == '2' || execType == 'F') && ordStatus == '2') {
					fix_get_string(fix_get_field(msg, OrderID), m_orderIdStrBuff, 64);
					fix_get_string(fix_get_field(msg, ClOrdID), m_clOrdIdStrBuff, 64);
					char side = fix_get_field(msg, Side)->string_value[0];
					double fillPrice = fix_get_field(msg, AvgPx)->float_value;

					m_systemLogger->info("[{}] Filled Px: {} id: {}", m_brokerStr, fillPrice, m_clOrdIdStrBuff);

					std::thread([=, mongoDriver = &m_mongoDriver, broker = m_brokerStr] {
						mongoDriver->record(m_clOrdIdStrBuff, m_orderIdStrBuff, side, fillPrice, broker);
					}).detach();
				}

				if (execType == '8' || execType == 'H') {
					char text[512];
					msg_string(text, msg, 512);
					m_consoleLogger->error("[{}] Trade Office Order FAILED {}", m_brokerStr, text);
					m_systemLogger->error("[{}] Cancelled id: {}", m_brokerStr, m_clOrdIdStrBuff);
				}
			}

				break;

			default:
				char text[512];
				msg_string(text, msg, 512);
				m_consoleLogger->error("[{}] Trade Office Unhandled FAILED {}", m_brokerStr, text);
				break;
		}
	});
}

void FIXTradeOffice::initiate() {
	m_fixSession.initiate();
	m_inRingBuffer->addGatingSequences({m_orderEventPoller->sequence()});
	m_consoleLogger->info("[{}] Trade Office OK", m_brokerStr);
}

void FIXTradeOffice::terminate() {
	m_fixSession.terminate();
	m_inRingBuffer->removeGatingSequence(m_orderEventPoller->sequence());
}

