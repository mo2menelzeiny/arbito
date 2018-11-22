
#include <FIXMarketOffice.h>

#include "FIXMarketOffice.h"

FIXMarketOffice::FIXMarketOffice(
		const char *broker,
		double quantity,
		int publicationPort,
		const char *publicationHost,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
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
    ) {
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
}

void FIXMarketOffice::start() {
	m_running = true;
	m_worker = std::thread(&FIXMarketOffice::work, this);
}

void FIXMarketOffice::stop() {
	m_running = false;
	m_worker.join();
}

void FIXMarketOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "market-office");

	auto systemLogger = spdlog::get("system");

	aeron::Context aeronContext;

	aeronContext
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

	auto publicationId = aeronClient->addExclusivePublication(m_publicationURI, 1);

	auto publication = aeronClient->findExclusivePublication(publicationId);
	while (!publication) {
		std::this_thread::yield();
		publication = aeronClient->findExclusivePublication(publicationId);
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

		if (m_quantity > offerQty || m_quantity > bidQty) return;

		marketData
				.bid(bid)
				.offer(offer);

		while (publication->offer(atomicBuffer, 0, encodedLength) < -1);

	});

	while (m_running) {
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
				m_fixSession.send(&m_MDRFixMessage);
				systemLogger->info("Market Office OK");
			} catch (std::exception &ex) {
				m_fixSession.terminate();
				systemLogger->error("Market Office {}", ex.what());
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}

			continue;
		}

		m_fixSession.poll(onMessageHandler);
	}

	m_fixSession.terminate();
}
