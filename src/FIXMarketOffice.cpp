
#include "FIXMarketOffice.h"

FIXMarketOffice::FIXMarketOffice(
		std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &inRingBuffer,
		int cpuset,
		const char *broker,
		double quantity,
		const char *host,
		int port,
		const char *username,
		const char *password,
		const char *sender,
		const char *target,
		int heartbeat
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
    ) {
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
	CPU_SET(m_cpuset, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "market-office");

	auto consoleLogger = spdlog::get("console");
	auto systemLogger = spdlog::get("system");

	long sequence = 0;

	auto offerIdx = strcmp(m_broker, "LMAX") ? 4 : 2;
	auto offerQtyIdx = offerIdx - 1;

	auto onMessageHandler = OnMessageHandler([&](struct fix_message *msg) {
		if (msg->type != FIX_MSG_TYPE_MARKET_DATA_SNAPSHOT_FULL_REFRESH) return;

		double bid = fix_get_float(msg, MDEntryPx, 0.0);
		double bidQty = fix_get_float(msg, MDEntrySize, 0.0);
		double offer = fix_get_field_at(msg, msg->nr_fields - offerIdx)->float_value;
		double offerQty = fix_get_field_at(msg, msg->nr_fields - offerQtyIdx)->float_value;

		if (m_quantity > offerQty || m_quantity > bidQty) return;

		++sequence;

		auto nextSequence = m_inRingBuffer->next();
		(*m_inRingBuffer)[nextSequence].bid = bid;
		(*m_inRingBuffer)[nextSequence].offer = offer;
		(*m_inRingBuffer)[nextSequence].sequence = sequence;
		m_inRingBuffer->publish(nextSequence);

		systemLogger->info("[{}][{}] bid: {} offer: {}", m_broker, sequence, bid, offer);
	});

	while (m_running) {
		if (!m_fixSession.isActive()) {

			try {
				m_fixSession.initiate();
				m_fixSession.send(&m_MDRFixMessage);
				consoleLogger->info("[{}] Market Office OK", m_broker);
			} catch (std::exception &ex) {
				m_fixSession.terminate();
				consoleLogger->error("[{}] Market Office {}", m_broker, ex.what());
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}

			continue;
		}

		m_fixSession.poll(onMessageHandler);
	}

	m_fixSession.terminate();
}
