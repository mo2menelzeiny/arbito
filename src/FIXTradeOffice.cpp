
#include "FIXTradeOffice.h"

FIXTradeOffice::FIXTradeOffice(
		std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &inRingBuffer,
		int cpuset,
		const char *broker,
		double quantity,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat,
		const char *dbUri,
		const char *dbName
) : m_inRingBuffer(inRingBuffer),
    m_cpuset(cpuset),
    m_broker(broker),
    m_quantity(quantity),
    m_fixSession(
		    host,
		    port,
		    username,
		    password,
		    sender,
		    target,
		    heartbeat
    ),
    m_mongoDriver(
		    dbUri,
		    dbName,
		    "coll_orders"
    ) {
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
				FIX_CHAR_FIELD(OrdType, '1')
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
				FIX_CHAR_FIELD(OrdType, '1')
		};

		size = ARRAY_SIZE(NOSBFields);
		m_NOSBFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSBFields, NOSBFields, size * sizeof(fix_field));
		m_NOSBFixMessage.nr_fields = size;
	}

	if (!strcmp(broker, "SWISSQUOTE")) {
		struct fix_field NOSSFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-SELL"),
				FIX_STRING_FIELD(Symbol, "EUR/USD"),
				FIX_CHAR_FIELD(Side, '2'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
		};
		unsigned long size = ARRAY_SIZE(NOSSFields);
		m_NOSSFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSSFields, NOSSFields, size * sizeof(fix_field));
		m_NOSSFixMessage.nr_fields = size;

		struct fix_field NOSBFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(Symbol, "EUR/USD"),
				FIX_CHAR_FIELD(Side, '1'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
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
				FIX_STRING_FIELD(SecurityType, "CASH"),
				FIX_CHAR_FIELD(Side, '2'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
				FIX_STRING_FIELD(Account, "U01038"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "IDEALPRO")
		};
		unsigned long size = ARRAY_SIZE(NOSSFields);
		m_NOSSFields = (fix_field *) malloc(size * sizeof(fix_field));
		memcpy(m_NOSSFields, NOSSFields, size * sizeof(fix_field));
		m_NOSSFixMessage.nr_fields = size;

		struct fix_field NOSBFields[] = {
				FIX_STRING_FIELD(ClOrdID, "NEW-ORDER-SINGLE-BUY"),
				FIX_STRING_FIELD(Symbol, "EUR"),
				FIX_STRING_FIELD(Currency, "USD"),
				FIX_STRING_FIELD(SecurityType, "CASH"),
				FIX_CHAR_FIELD(Side, '1'),
				FIX_STRING_FIELD(TransactTime, ""),
				FIX_FLOAT_FIELD(OrderQty, quantity),
				FIX_CHAR_FIELD(OrdType, '1'),
				FIX_STRING_FIELD(Account, "U01038"),
				FIX_INT_FIELD(CustomerOrFirm, 0),
				FIX_STRING_FIELD(ExDestination, "IDEALPRO")
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
}

void FIXTradeOffice::start() {
	m_running = true;
	m_worker = std::thread(&FIXTradeOffice::work, this);
}

void FIXTradeOffice::stop() {
	m_running = false;
	m_worker.join();
}

void FIXTradeOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(m_cpuset, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "trade-office");

	auto consoleLogger = spdlog::get("console");
	auto systemLogger = spdlog::get("system");

	BrokerEnum brokerEnum;

	if (!strcmp(m_broker, "LMAX")) {
		brokerEnum = LMAX;
	}

	if (!strcmp(m_broker, "IB")) {
		brokerEnum = IB;
	}

	if (!strcmp(m_broker, "SWISSQUOTE")) {
		brokerEnum = SWISSQUOTE;
	}

	char clOrdIdStr[64];
	char orderIdStr[64];

	auto businessPoller = m_inRingBuffer->newPoller();

	m_inRingBuffer->addGatingSequences({businessPoller->sequence()});

	auto businessHandler = [&](BusinessEvent &event, int64_t seq, bool endOfBatch) -> bool {
		sprintf(clOrdIdStr, "%lu", event.id);

		if (event.buy == brokerEnum) {
			fix_get_field(&m_NOSBFixMessage, ClOrdID)->string_value = clOrdIdStr;
			fix_get_field(&m_NOSBFixMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_NOSBFixMessage);
		}

		if (event.sell == brokerEnum) {
			fix_get_field(&m_NOSSFixMessage, ClOrdID)->string_value = clOrdIdStr;
			fix_get_field(&m_NOSSFixMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_NOSSFixMessage);
		}

		const char *side = event.buy == brokerEnum ? "BUY" : "SELL";

		systemLogger->info("[{}] {} id: {}", m_broker, side, clOrdIdStr);
		consoleLogger->info("[{}] {}", m_broker, side);

		return false;
	};

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT: {
				char execType = fix_get_field(msg, ExecType)->string_value[0];
				char ordStatus = fix_get_field(msg, OrdStatus)->string_value[0];

				if ((execType == '2' || execType == 'F') && ordStatus == '2') {
					fix_get_string(fix_get_field(msg, OrderID), orderIdStr, 64);
					fix_get_string(fix_get_field(msg, ClOrdID), clOrdIdStr, 64);
					char side = fix_get_field(msg, Side)->string_value[0];
					double fillPrice = fix_get_field(msg, AvgPx)->float_value;

					systemLogger->info("[{}] Filled Px: {} id: {}", m_broker, fillPrice, clOrdIdStr);

					std::thread([=, mongoDriver = &m_mongoDriver, broker = m_broker] {
						mongoDriver->record(clOrdIdStr, orderIdStr, side, fillPrice, broker);
					}).detach();
				}

				if (execType == '8' || execType == 'H') {
					// TODO: Handle failed order execution
					systemLogger->error("[{}] Cancelled id: {}", m_broker, clOrdIdStr);
					consoleLogger->error("[{}] Trade Office Order FAILED", m_broker);
					fprintmsg_iov(stdout, msg);
				}
			}

				break;

			default:
				consoleLogger->error("[{}] Trade Office Unhandled FAILED", m_broker);
				fprintmsg_iov(stdout, msg);

				break;
		}
	});

	std::mt19937_64 randomGenerator(std::random_device{}());
	char randIdStr[64];
	bool isOrderDelayed = false, flag = false;
	time_t orderDelay = 60;
	time_t orderDelayStart = time(nullptr);

	while (m_running) {
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
				consoleLogger->info("[{}] Trade Office OK", m_broker);
			} catch (std::exception &ex) {
				m_fixSession.terminate();
				consoleLogger->error("[{}] Trade office {}", m_broker, ex.what());
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}

			continue;
		}

		m_fixSession.poll(onMessageHandler);
		businessPoller->poll(businessHandler);

		// FIXME: IB QA FIX session script

		if (isOrderDelayed && ((time(nullptr) - orderDelayStart) < orderDelay)) continue;

		isOrderDelayed = true;
		orderDelayStart = time(nullptr);

		sprintf(randIdStr, "%lu", randomGenerator());

		if (flag) {
			flag = false;
			fix_get_field(&m_NOSBFixMessage, ClOrdID)->string_value = randIdStr;
			fix_get_field(&m_NOSBFixMessage, TransactTime)->string_value = m_fixSession.strNow();
			m_fixSession.send(&m_NOSBFixMessage);
			consoleLogger->info("[{}] BUY", m_broker);
			continue;
		}

		flag = true;
		fix_get_field(&m_NOSSFixMessage, ClOrdID)->string_value = randIdStr;
		fix_get_field(&m_NOSSFixMessage, TransactTime)->string_value = m_fixSession.strNow();
		m_fixSession.send(&m_NOSSFixMessage);
		consoleLogger->info("[{}] SELL", m_broker);
	}

	m_inRingBuffer->removeGatingSequence(businessPoller->sequence());
}
