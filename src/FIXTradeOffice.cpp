
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
		int heartbeat,
		const char *DBUri,
		const char *DBName
) : m_broker(broker),
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
		    broker,
		    DBUri,
		    DBName,
		    "coll_orders"
    ) {
	sprintf(m_subscriptionURI, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPort);

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
	CPU_SET(2, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "trade-office");

	auto systemLogger = spdlog::get("system");

	aeron::Context aeronContext;

	aeronContext
			.useConductorAgentInvoker(true)
			.availableImageHandler([&](aeron::Image &image) {
				systemLogger->info("{} available", image.sourceIdentity());
			})
			.unavailableImageHandler([&](aeron::Image &image) {
				systemLogger->error("{} unavailable", image.sourceIdentity());
			})
			.errorHandler([&](const std::exception &ex) {
				systemLogger->error("{}", ex.what());
			});

	auto aeronClient = std::make_shared<aeron::Aeron>(aeronContext);

	aeronClient->conductorAgentInvoker().start();

	auto subscriptionId = aeronClient->addSubscription(m_subscriptionURI, 1);

	auto subscription = aeronClient->findSubscription(subscriptionId);
	while (!subscription) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		subscription = aeronClient->findSubscription(subscriptionId);
	}

	sbe::MessageHeader tradeDataHeader;
	sbe::TradeData tradeData;

	auto fragmentHandler = [&](
			aeron::AtomicBuffer &buffer,
			aeron::index_t offset,
			aeron::index_t length,
			const aeron::Header &header
	) {
		auto bufferLength = static_cast<const uint64_t>(length);
		auto messageBuffer = reinterpret_cast<char *>(buffer.buffer() + offset);

		tradeDataHeader.wrap(messageBuffer, 0, 0, bufferLength);
		tradeData.wrapForDecode(
				messageBuffer,
				sbe::MessageHeader::encodedLength(),
				tradeDataHeader.blockLength(),
				tradeDataHeader.version(),
				bufferLength
		);

		char clOrdId[64];
		sprintf(clOrdId, "%lu", tradeData.id());

		switch (tradeData.side()) {
			case '1':
				fix_get_field(&m_NOSBFixMessage, ClOrdID)->string_value = clOrdId;
				fix_get_field(&m_NOSBFixMessage, TransactTime)->string_value = m_fixSession.strNow();
				m_fixSession.send(&m_NOSBFixMessage);
				break;
			case '2':
				fix_get_field(&m_NOSSFixMessage, ClOrdID)->string_value = clOrdId;
				fix_get_field(&m_NOSSFixMessage, TransactTime)->string_value = m_fixSession.strNow();
				m_fixSession.send(&m_NOSSFixMessage);
				break;
			default:
				break;
		}

		systemLogger->info("{}", tradeData.side() == '1' ? "BUY" : "SELL");
	};

	auto fragmentAssembler = aeron::FragmentAssembler(fragmentHandler);

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		switch (msg->type) {
			case FIX_MSG_TYPE_EXECUTION_REPORT: {
				char execType = fix_get_field(msg, ExecType)->string_value[0];

				if (execType == '2' || execType == 'F') {
					char orderId[64];
					char clOrdId[64];
					fix_get_string(fix_get_field(msg, OrderID), orderId, 64);
					fix_get_string(fix_get_field(msg, ClOrdID), clOrdId, 64);
					char side = fix_get_field(msg, Side)->string_value[0];
					double fillPrice = fix_get_field(msg, AvgPx)->float_value;

					m_mongoDriver.recordExecution(clOrdId, orderId, side, fillPrice);
				}

				if (execType == '8') {
					// TODO: Handle failed order execution
					systemLogger->error("Order FAILED {}", tradeData.id());
				}
			}

				break;
			default:
				return;
		}
	});

	while (m_running) {
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
				systemLogger->info("Trade Office OK");
			} catch (std::exception &ex) {
				m_fixSession.terminate();
				systemLogger->error("Trade office {}", ex.what());
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}

			continue;
		}

		subscription->poll(fragmentAssembler.handler(), 1);
		m_fixSession.poll(onMessageHandler);
		aeronClient->conductorAgentInvoker().invoke();
	}
}
