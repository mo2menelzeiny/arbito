
#include <FIXTradeOffice.h>

FIXTradeOffice::FIXTradeOffice(
		std::shared_ptr<Disruptor::RingBuffer<OrderEvent>> &inRingBuffer,
		std::shared_ptr<Disruptor::RingBuffer<ExecutionEvent>> &outRingBuffer,
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
    m_outRingBuffer(outRingBuffer),
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

	if (m_brokerEnum == Broker::LMAX) {
		struct fix_field limitOrderFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-SELL"),
				FIX_STRING_FIELD(SecurityID, "4001"),
				FIX_STRING_FIELD(SecurityIDSource, "8"),
				FIX_CHAR_FIELD(Side, '0'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_CHAR_FIELD(TimeInForce, '4'),
				FIX_FLOAT_FIELD(Price, 0)
		};
		unsigned long size = ARRAY_SIZE(limitOrderFields);
		m_limitOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_limitOrderFields, limitOrderFields, size * sizeof(fix_field));
		m_limitOrderMessage.nr_fields = size;

		struct fix_field marketOrderFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(SecurityID, "4001"),
				FIX_STRING_FIELD(SecurityIDSource, "8"),
				FIX_CHAR_FIELD(Side, '0'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
		};

		size = ARRAY_SIZE(marketOrderFields);
		m_marketOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_marketOrderFields, marketOrderFields, size * sizeof(fix_field));
		m_marketOrderMessage.nr_fields = size;
	}

	if (m_brokerEnum == Broker::IB) {
		struct fix_field limitOrderFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-SELL"),
				FIX_STRING_FIELD(Symbol, "EUR"),
				FIX_STRING_FIELD(Currency, "USD"),
				FIX_STRING_FIELD(SecurityType, "CFD"),
				FIX_CHAR_FIELD(Side, '0'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '2'),
				FIX_STRING_FIELD(Account, "U2707646"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "SMART"),
				FIX_CHAR_FIELD(TimeInForce, '3'),
				FIX_FLOAT_FIELD(Price, 0)
		};
		unsigned long size = ARRAY_SIZE(limitOrderFields);
		m_limitOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_limitOrderFields, limitOrderFields, size * sizeof(fix_field));
		m_limitOrderMessage.nr_fields = size;

		struct fix_field marketOrderFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(Symbol, "EUR"),
				FIX_STRING_FIELD(Currency, "USD"),
				FIX_STRING_FIELD(SecurityType, "CFD"),
				FIX_CHAR_FIELD(Side, '0'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
				FIX_STRING_FIELD(Account, "U2707646"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "SMART")
		};

		size = ARRAY_SIZE(marketOrderFields);
		m_marketOrderFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_marketOrderFields, marketOrderFields, size * sizeof(fix_field));
		m_marketOrderMessage.nr_fields = size;
	}

	m_limitOrderMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_limitOrderMessage.fields = m_limitOrderFields;

	m_marketOrderMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_marketOrderMessage.fields = m_marketOrderFields;

	m_orderEventHandler = [&](OrderEvent &event, int64_t seq, bool endOfBatch) -> bool {
		if (event.broker != m_brokerEnum) {
			return false;
		}

		sprintf(m_clOrdIdStrBuff, "%lu", event.id);

		if (event.order == OrderType::LIMIT && event.side == OrderSide::BUY) {
			fix_get_field(&m_limitOrderMessage, Side)->char_value = 49;
			fix_get_field(&m_limitOrderMessage, Price)->float_value = event.price;
			fix_get_field(&m_limitOrderMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_limitOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_limitOrderMessage);
		}

		if (event.order == OrderType::LIMIT && event.side == OrderSide::SELL) {
			fix_get_field(&m_limitOrderMessage, Side)->char_value = 50;
			fix_get_field(&m_limitOrderMessage, Price)->float_value = event.price;
			fix_get_field(&m_limitOrderMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_limitOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_limitOrderMessage);
		}

		if (event.order == OrderType::MARKET && event.side == OrderSide::BUY) {
			fix_get_field(&m_marketOrderMessage, Side)->char_value = 49;
			fix_get_field(&m_marketOrderMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_marketOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_marketOrderMessage);
		}

		if (event.order == OrderType::MARKET && event.side == OrderSide::SELL) {
			fix_get_field(&m_marketOrderMessage, Side)->char_value = 50;
			fix_get_field(&m_marketOrderMessage, ClOrdID)->string_value = m_clOrdIdStrBuff;
			fix_get_field(&m_marketOrderMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_marketOrderMessage);
		}

		m_systemLogger->info(
				"[{}] {} id: {}",
				m_brokerStr,
				event.side == OrderSide::BUY ? "BUY" : "SELL",
				m_clOrdIdStrBuff
		);

		return false;
	};

	m_onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT: {
				char ordStatus = fix_get_field(msg, OrdStatus)->string_value[0];
				char execType = fix_get_field(msg, ExecType)->string_value[0];
				char side = fix_get_field(msg, Side)->string_value[0];
				enum OrderSide orderSide = side == '1' ? OrderSide::BUY : OrderSide::SELL;
				double fillPrice = fix_get_field(msg, AvgPx)->float_value;
				fix_get_string(fix_get_field(msg, OrderID), m_orderIdStrBuff, 64);
				fix_get_string(fix_get_field(msg, ClOrdID), m_clOrdIdStrBuff, 64);

				if ((execType == '2' || execType == 'F') && ordStatus == '2') {
					auto nextSequence = m_outRingBuffer->next();
					(*m_outRingBuffer)[nextSequence].broker = m_brokerEnum;
					(*m_outRingBuffer)[nextSequence].side = orderSide;
					(*m_outRingBuffer)[nextSequence].isFilled = true;
					(*m_outRingBuffer)[nextSequence].id = stoul(m_clOrdIdStrBuff)
					m_outRingBuffer->publish(nextSequence);

					m_systemLogger->info("[{}] Order Filled Price: {}", m_brokerStr, fillPrice);

					std::thread([=, mongoDriver = &m_mongoDriver] {
						mongoDriver->record(m_clOrdIdStrBuff, m_orderIdStrBuff, side, fillPrice, m_brokerStr, true);
					}).detach();
				}

				if (execType == '8' || execType == 'H') {
					auto nextSequence = m_outRingBuffer->next();
					(*m_outRingBuffer)[nextSequence].broker = m_brokerEnum;
					(*m_outRingBuffer)[nextSequence].side = orderSide;
					(*m_outRingBuffer)[nextSequence].isFilled = false;
					(*m_outRingBuffer)[nextSequence].id = stoul(m_clOrdIdStrBuff)
					m_outRingBuffer->publish(nextSequence);

					char text[512];
					msg_string(text, msg, 512);
					m_consoleLogger->error("[{}] Trade Office Order FAILED {}", m_brokerStr, text);

					m_systemLogger->error("[{}] Order Rejected id: {} \n {}", m_brokerStr, m_clOrdIdStrBuff, text);

					std::thread([=, mongoDriver = &m_mongoDriver] {
						mongoDriver->record(m_clOrdIdStrBuff, m_orderIdStrBuff, side, fillPrice, m_brokerStr, false);
					}).detach();
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

