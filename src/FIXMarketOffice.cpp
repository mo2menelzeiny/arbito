
#include "FIXMarketOffice.h"

FIXMarketOffice::FIXMarketOffice(
		const char *broker,
		double spread,
		double quantity,
		const char *BOHost,
		int BOPort,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
) : m_broker(broker),
    m_spread(spread),
    m_quantity(quantity),
    m_BOHost(BOHost),
    m_BOPort(BOPort) {
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
	aeron::Context context;

	context.availableImageHandler([&](aeron::Image &image) {
	});

	context.unavailableImageHandler([&](aeron::Image &image) {
	});

	context.errorHandler([&](const std::exception &ex) {
	});

	auto client = std::make_shared<aeron::Aeron>(context);

	char uri[64];
	sprintf(uri, "aeron:udp?endpoint=%s:%i\n", m_BOHost, m_BOPort);

	auto publicationId = client->addExclusivePublication(uri, 1);

	auto publication = client->findExclusivePublication(publicationId);

	while (!publication) {
		std::this_thread::yield();
		publication = client->findExclusivePublication(publicationId);
	}

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);
	
	sbe::MessageHeader messageHeader;
	sbe::MarketData marketData;

	auto offerIdx = strcmp(m_broker, "LMAX") ? 4 : 2;
	auto offerQtyIdx = offerIdx - 1;
	
	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		if (msg->type != FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH) return;

		double bid = fix_get_float(msg, MDEntryPx, 0.0);
		double bidQty = fix_get_float(msg, MDEntrySize, 0.0);
		double offer = fix_get_field_at(msg, msg->nr_fields - offerIdx)->float_value;
		double offerQty = fix_get_field_at(msg, msg->nr_fields - offerQtyIdx)->float_value;

		if (m_spread < offer - bid || m_quantity > offerQty || m_quantity > bidQty) return;
		
		messageHeader
				.wrap(reinterpret_cast<char *>(buffer), 0, 0, 64)
				.blockLength(sbe::MarketData::sbeBlockLength())
				.templateId(sbe::MarketData::sbeTemplateId())
				.schemaId(sbe::MarketData::sbeSchemaId())
				.version(sbe::MarketData::sbeSchemaVersion());

		marketData
				.wrapForEncode(reinterpret_cast<char *>(buffer), sbe::MessageHeader::encodedLength(), 64)
				.bid(bid)
				.offer(offer);

		aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe::MessageHeader::encodedLength();

		while (publication->offer(atomicBuffer, 0, len) < -1);

	});

	auto doWorkHandler = DoWorkHandler([&](struct fix_session *session) {
		// no work needed in market session
	});

	while (true) {
		m_fixSession->poll(doWorkHandler, onMessageHandler);
	}
}
