
#include <sbe/TradeData.h>
#include "FIXTradeOffice.h"

FIXTradeOffice::FIXTradeOffice(
		const char *broker,
		double quantity,
		int subscriptionPort,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
) : m_broker(broker),
    m_quantity(quantity),
    m_subscriptionPort(subscriptionPort) {
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

	m_NOSSFixMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_NOSSFixMessage.fields = m_NOSSFields;

	m_NOSBFixMessage.type = FIX_MSG_TYPE_NEW_ORDER_SINGLE;
	m_NOSBFixMessage.fields = m_NOSBFields;

	m_fixSession = new FIXSession(
			host,
			port,
			username,
			password,
			sender,
			target,
			heartbeat,
			[&](const std::exception &ex) {
				printf("%s\n", ex.what());
			},
			[&](struct fix_session *session) {
				// do nothing on start
			}
	);
}

void FIXTradeOffice::start() {
	m_fixSession->initiate();
	auto worker = std::thread(&FIXTradeOffice::work, this);
	worker.detach();
}

void FIXTradeOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(2, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "trade");


	aeron::Context context;

	context.availableImageHandler([&](aeron::Image &image) {
	});

	context.unavailableImageHandler([&](aeron::Image &image) {
	});

	context.errorHandler([&](const std::exception &ex) {
	});

	auto client = std::make_shared<aeron::Aeron>(context);

	char uri[64];
	sprintf(uri, "aeron:udp?endpoint=0.0.0.0:%i\n", m_subscriptionPort);

	auto subscriptionId = client->addSubscription(uri, 1);

	auto subscription = client->findSubscription(subscriptionId);

	while (!subscription) {
		std::this_thread::yield();
		subscription = client->findSubscription(subscriptionId);
	}

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);

	sbe::MessageHeader messageHeader;
	sbe::TradeData tradeData;
	
	bool hasOrder = false;
	
	auto fragmentHandler = [&](
			aeron::AtomicBuffer &buffer,
			aeron::index_t offset,
			aeron::index_t length,
			const aeron::Header &header
	) {
		messageHeader.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, 64);
		tradeData.wrapForDecode(
				reinterpret_cast<char *>(buffer.buffer() + offset),
				sbe::MessageHeader::encodedLength(),
				messageHeader.blockLength(),
				messageHeader.version(),
				64
		);
		
		hasOrder = true;
	};
	
	auto fragmentAssembler = aeron::FragmentAssembler(fragmentHandler);

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT:
				char execType = fix_get_field(msg, ExecType)->string_value[0];

				if (execType == '2' || execType == 'F') {
					char orderId[64];
					char clOrdId[64];
					fix_get_string(fix_get_field(msg, OrderID), orderId, 64);
					fix_get_string(fix_get_field(msg, ClOrdID), clOrdId, 64);
					char side = fix_get_field(msg, Side)->string_value[0];
					double avgPx = fix_get_field(msg, AvgPx)->float_value;
				}
				
				if(execType == '8') {
					// failed order
				}

				break;
			default:
				return;
		}
	});

	auto doWorkHandler = DoWorkHandler([&](struct fix_session *session) {
		if (!hasOrder) return;
		
		char id[64];
		sprintf(id, "%li", tradeData.id());
		
		switch (tradeData.side()) {
			case 2: // SELL
				fix_get_field(&m_NOSSFixMessage, ClOrdID)->string_value = id;
				fix_get_field(&m_NOSSFixMessage, TransactTime)->string_value = session->str_now;
				fix_session_send(session, &m_NOSSFixMessage, 0);
				break;
			case 1: // BUY
				fix_get_field(&m_NOSBFixMessage, ClOrdID)->string_value = id;
				fix_get_field(&m_NOSBFixMessage, TransactTime)->string_value = session->str_now;
				fix_session_send(session, &m_NOSBFixMessage, 0);
				break;
			default:
				// exception
				break;
		}
		
		hasOrder = false;
	});
	
	while (true) {
		subscription->poll(fragmentAssembler.handler(), 10);
		m_fixSession->poll(doWorkHandler, onMessageHandler);
	}
}
