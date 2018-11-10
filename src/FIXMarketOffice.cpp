
#include "FIXMarketOffice.h"

FIXMarketOffice::FIXMarketOffice(
		const char *broker,
		double spread,
		double quantity,
		const char *publicationHost,
		int publicationPort,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
) : m_broker(broker),
    m_spread(spread),
    m_quantity(quantity) {
	sprintf(m_publicationURI, "aeron:udp?endpoint=%s:%i\n", publicationHost, publicationPort);

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
		m_MDRFields = (fix_field *) malloc(size * sizeof(fix_field));
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
		m_MDRFields = (fix_field *) malloc(size * sizeof(fix_field));
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
	auto worker = std::thread(&FIXMarketOffice::work, this);
	worker.detach();
}

void FIXMarketOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "market");

	aeron::Context context;

	context.availableImageHandler([&](aeron::Image &image) {
	});

	context.unavailableImageHandler([&](aeron::Image &image) {
	});

	context.errorHandler([&](const std::exception &ex) {
	});

	auto client = std::make_shared<aeron::Aeron>(context);

	auto publicationId = client->addExclusivePublication(m_publicationURI, 1);

	auto publication = client->findExclusivePublication(publicationId);

	while (!publication) {
		std::this_thread::yield();
		publication = client->findExclusivePublication(publicationId);
	}

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);

	auto messageBuffer = reinterpret_cast<char *>(buffer);

	sbe::MessageHeader messageHeader;
	sbe::MarketData marketData;

	messageHeader
			.wrap(messageBuffer, 0, 0, 64)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	marketData.wrapForEncode(messageBuffer, sbe::MessageHeader::encodedLength(), 64);

	auto encodedLength = static_cast<aeron::index_t>(sbe::MessageHeader::encodedLength() + marketData.encodedLength());

	auto offerIdx = strcmp(m_broker, "LMAX") ? 4 : 2;
	auto offerQtyIdx = offerIdx - 1;

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		if (msg->type != FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH) return;

		double bid = fix_get_float(msg, MDEntryPx, 0.0);
		double bidQty = fix_get_float(msg, MDEntrySize, 0.0);
		double offer = fix_get_field_at(msg, msg->nr_fields - offerIdx)->float_value;
		double offerQty = fix_get_field_at(msg, msg->nr_fields - offerQtyIdx)->float_value;

		if (m_spread < offer - bid || m_quantity > offerQty || m_quantity > bidQty) return;

		marketData
				.bid(bid)
				.offer(offer);

		while (publication->offer(atomicBuffer, 0, encodedLength) < -1);

	});

	auto doWorkHandler = DoWorkHandler([&](struct fix_session *session) {
		// no work needed in market session
	});

	while (true) {
		m_fixSession->poll(doWorkHandler, onMessageHandler);
	}
}
