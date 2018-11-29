
#include "CentralOffice.h"

CentralOffice::CentralOffice(
		int publicationPortA,
		const char *publicationHostA,
		int publicationPortB,
		const char *publicationHostB,
		int subscriptionPortA,
		int subscriptionPortB,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose,
		const char *dbUri,
		const char *dbName
) : m_orderDelaySec(orderDelaySec),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose),
    m_mongoDriver(
		    "CENTRAL",
		    dbUri,
		    dbName,
		    "coll_triggers"
    ) {
	sprintf(m_subscriptionURIA, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPortA);
	sprintf(m_subscriptionURIB, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPortB);
	sprintf(m_publicationURIA, "aeron:udp?endpoint=%s:%i\n", publicationHostA, publicationPortA);
	sprintf(m_publicationURIB, "aeron:udp?endpoint=%s:%i\n", publicationHostB, publicationPortB);
}

void CentralOffice::start() {
	m_running = true;
	m_worker = std::thread(&CentralOffice::work, this);
}

void CentralOffice::stop() {
	m_running = false;
	m_worker.join();
}

void CentralOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "central-office");

	long sequence = 0;

	double bidA = -99, offerA = 99;
	double bidB = -99, offerB = 99;

	int ordersCount = stoi(getenv("ORDERS_COUNT"));

	TriggerDifference currentDiff = DIFF_NONE;

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_A")) currentDiff = DIFF_A;

	if (!strcmp(getenv("CURRENT_DIFF"), "DIFF_B")) currentDiff = DIFF_B;

	TriggerDifference currentOrder = DIFF_NONE;

	bool isOrderDelayed = false;
	time_t orderDelay = m_orderDelaySec;
	time_t orderDelayStart = time(nullptr);

	auto systemLogger = spdlog::get("system");
	auto marketLogger = spdlog::daily_logger_st("market", "market_log");

	std::mt19937_64 randomGenerator(std::random_device{}());

	aeron::Context aeronContext;

	aeronContext
			.useConductorAgentInvoker(true)
			.availableImageHandler([&](aeron::Image &image) {
				systemLogger->info("{} available", image.sourceIdentity());
			})

			.unavailableImageHandler([&](aeron::Image &image) {
				bidA = -99, offerA = 99;
				bidB = -99, offerB = 99;
				systemLogger->error("{} unavailable", image.sourceIdentity());
			})

			.errorHandler([&](const std::exception &ex) {
				systemLogger->error("{}", ex.what());
			});

	auto aeronClient = std::make_shared<aeron::Aeron>(aeronContext);

	aeronClient->conductorAgentInvoker().start();

	auto publicationIdA = aeronClient->addExclusivePublication(m_publicationURIA, 1);
	auto publicationA = aeronClient->findExclusivePublication(publicationIdA);
	while (!publicationA) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		publicationA = aeronClient->findExclusivePublication(publicationIdA);
	}

	auto publicationIdB = aeronClient->addExclusivePublication(m_publicationURIB, 1);
	auto publicationB = aeronClient->findExclusivePublication(publicationIdB);
	while (!publicationB) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		publicationB = aeronClient->findExclusivePublication(publicationIdB);
	}

	auto subscriptionIdA = aeronClient->addSubscription(m_subscriptionURIA, 1);
	auto subscriptionA = aeronClient->findSubscription(subscriptionIdA);
	while (!subscriptionA) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		subscriptionA = aeronClient->findSubscription(subscriptionIdA);
	}

	auto subscriptionIdB = aeronClient->addSubscription(m_subscriptionURIB, 1);
	auto subscriptionB = aeronClient->findSubscription(subscriptionIdB);
	while (!subscriptionB) {
		std::this_thread::yield();
		aeronClient->conductorAgentInvoker().invoke();
		subscriptionB = aeronClient->findSubscription(subscriptionIdB);
	}

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);
	auto tradeDataBuffer = reinterpret_cast<char *>(buffer);

	sbe::MessageHeader tradeDataHeader;
	sbe::TradeData tradeData;

	tradeDataHeader
			.wrap(tradeDataBuffer, 0, 0, 64)
			.blockLength(sbe::TradeData::sbeBlockLength())
			.templateId(sbe::TradeData::sbeTemplateId())
			.schemaId(sbe::TradeData::sbeSchemaId())
			.version(sbe::TradeData::sbeSchemaVersion());

	tradeData.wrapForEncode(tradeDataBuffer, sbe::MessageHeader::encodedLength(), 64);

	auto encodedLength = static_cast<aeron::index_t>(sbe::MessageHeader::encodedLength() + tradeData.encodedLength());

	char randIdStr[64];

	auto handleTriggers = [&] {
		if (isOrderDelayed && ((time(nullptr) - orderDelayStart) < orderDelay)) return;

		isOrderDelayed = false;

		bool canOpen = ordersCount < m_maxOrders;

		double diffA = bidA - offerB;
		double diffB = bidB - offerA;

		switch (currentDiff) {
			case DIFF_A:
				if (canOpen && diffA >= m_diffOpen) {
					currentOrder = DIFF_A;
					++ordersCount;
					break;
				}

				if (diffB >= m_diffClose) {
					currentOrder = DIFF_B;
					--ordersCount;
					break;
				}

			case DIFF_B:
				if (canOpen && diffB >= m_diffOpen) {
					currentOrder = DIFF_B;
					++ordersCount;
					break;
				}

				if (diffA >= m_diffClose) {
					currentOrder = DIFF_A;
					--ordersCount;
					break;
				}

			case DIFF_NONE:
				if (diffA >= m_diffOpen) {
					currentDiff = DIFF_A;
					currentOrder = DIFF_A;
					++ordersCount;
					break;
				}

				if (diffB >= m_diffOpen) {
					currentDiff = DIFF_B;
					currentOrder = DIFF_B;
					++ordersCount;
					break;
				}

		}

		if (ordersCount == 0) {
			currentDiff = DIFF_NONE;
		}

		if (currentOrder == DIFF_NONE) return;

		const char *orderType = currentOrder == currentDiff ? "OPEN" : "CLOSE";

		auto randomId = randomGenerator();
		tradeData.id(randomId);
		sprintf(randIdStr, "%lu", randomId);

		switch (currentOrder) {
			case DIFF_A:
				tradeData.side('2');
				while (publicationA->offer(atomicBuffer, 0, encodedLength) < -1);

				tradeData.side('1');
				while (publicationB->offer(atomicBuffer, 0, encodedLength) < -1);

				std::thread([=, mongoDriver = &m_mongoDriver] {
					mongoDriver->record(randIdStr, bidA, offerB, orderType);
				}).detach();

				break;

			case DIFF_B:
				tradeData.side('1');
				while (publicationA->offer(atomicBuffer, 0, encodedLength) < -1);

				tradeData.side('2');
				while (publicationB->offer(atomicBuffer, 0, encodedLength) < -1);

				std::thread([=, mongoDriver = &m_mongoDriver] {
					mongoDriver->record(randIdStr, bidB, offerA, orderType);
				}).detach();

				break;
		}

		bidA = -99;
		offerA = 99;
		bidB = -99;
		offerB = 99;
		currentOrder = DIFF_NONE;
		orderDelayStart = time(nullptr);
		isOrderDelayed = true;

		systemLogger->info("{}", orderType);
	};


	auto fragmentHandlerA = [&](
			aeron::AtomicBuffer &buffer,
			aeron::index_t offset,
			aeron::index_t length,
			const aeron::Header &header
	) {
		auto bufferLength = static_cast<const uint64_t>(length);
		auto messageBuffer = reinterpret_cast<char *>(buffer.buffer() + offset);

		sbe::MessageHeader marketDataHeader;
		sbe::MarketData marketData;

		marketDataHeader.wrap(messageBuffer, 0, 0, bufferLength);
		marketData.wrapForDecode(
				messageBuffer,
				sbe::MessageHeader::encodedLength(),
				marketDataHeader.blockLength(),
				marketDataHeader.version(),
				bufferLength
		);

		bidA = marketData.bid();
		offerA = marketData.offer();

		++sequence;

		marketLogger->info("[slough][{}] bid: {} offer: {} RSeq: {}", sequence, bidA, offerA, marketData.timestamp());
	};

	auto fragmentHandlerB = [&](
			aeron::AtomicBuffer &buffer,
			aeron::index_t offset,
			aeron::index_t length,
			const aeron::Header &header
	) {
		auto bufferLength = static_cast<const uint64_t>(length);
		auto messageBuffer = reinterpret_cast<char *>(buffer.buffer() + offset);

		sbe::MessageHeader marketDataHeader;
		sbe::MarketData marketData;

		marketDataHeader.wrap(messageBuffer, 0, 0, bufferLength);
		marketData.wrapForDecode(
				messageBuffer,
				sbe::MessageHeader::encodedLength(),
				marketDataHeader.blockLength(),
				marketDataHeader.version(),
				bufferLength
		);

		bidB = marketData.bid();
		offerB = marketData.offer();

		++sequence;

		marketLogger->info("[zurich][{}] bid: {} offer: {} RSeq: {}", sequence, bidB, offerB, marketData.timestamp());
	};

	auto fragmentAssemblerA = aeron::FragmentAssembler(fragmentHandlerA);
	auto fragmentAssemblerB = aeron::FragmentAssembler(fragmentHandlerB);

	systemLogger->info("Central Office OK");

	while (m_running) {
		aeronClient->conductorAgentInvoker().invoke();
		subscriptionA->poll(fragmentAssemblerA.handler(), 1);
		subscriptionB->poll(fragmentAssemblerB.handler(), 1);
		handleTriggers();
	}
}
