
#include "FIXMarketOffice.h"

FIXMarketOffice::FIXMarketOffice(
		const char *broker,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
): m_broker(broker) {
	if (!strcmp(broker, "LMAX")) {
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
		m_MDRFields = (fix_field*) malloc( size * sizeof(fix_field));
		memcpy(m_MDRFields, fields, size * sizeof(fix_field));
		m_MDRFixMessage.nr_fields = size;
	}

	if (!strcmp(broker, "SWISSQUOTE")) {
		struct fix_field fields[] = {
				FIX_STRING_FIELD(MDReqID, "MARKET-DATA-REQUEST"),
				FIX_CHAR_FIELD(SubscriptionRequestType, '1'),
				FIX_INT_FIELD(MarketDepth, 1),
				FIX_INT_FIELD(MDUpdateType, 0),
				FIX_INT_FIELD(NoMDEntryTypes, 2),
				FIX_CHAR_FIELD(MDEntryType, '0'),
				FIX_CHAR_FIELD(MDEntryType, '1'),
				FIX_INT_FIELD(NoRelatedSym, 1),
				FIX_STRING_FIELD(Symbol, "EUR/USD")
		};
		unsigned long size = ARRAY_SIZE(fields);
		m_MDRFields = (fix_field*) malloc(size * sizeof(fix_field));
		memcpy(m_MDRFields, fields, size * sizeof(fix_field));
		m_MDRFixMessage.nr_fields = size;
	}

	m_MDRFixMessage.type = FIX_MSG_TYPE_MARKET_DATA_REQUEST;
	m_MDRFixMessage.fields = m_MDRFields;

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
				fix_session_send(session, &m_MDRFixMessage, 0);
			}
	);
}

void FIXMarketOffice::start() {
	m_fixSession->initiate();
	auto pollThread = std::thread(&FIXMarketOffice::poll, this);
	pollThread.detach();
}

void FIXMarketOffice::poll() {
	auto doWorkHandler = DoWorkHandler([&](struct fix_session *session) {
		// no work needed in market session
	});

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		printf("%s\n", msg->begin_string);
	});

	while (true) {
		m_fixSession->poll(doWorkHandler, onMessageHandler);
	}
}
