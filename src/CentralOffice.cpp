
#include "CentralOffice.h"

CentralOffice::CentralOffice(
		int publicationPortA,
		const char *publicationHostA,
		int publicationPortB,
		const char *publicationHostB,
		int subscriptionPortA,
		int subscriptionPortB,
		int windowMs,
		int orderDelaySec,
		int maxOrders,
		double diffOpen,
		double diffClose
) : m_windowMs(windowMs),
    m_orderDelaySec(orderDelaySec),
    m_maxOrders(maxOrders),
    m_diffOpen(diffOpen),
    m_diffClose(diffClose) {
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

	auto systemLogger = spdlog::get("system");
	auto marketLogger = spdlog::daily_logger_st("market", "market_log");

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

	auto publicationIdA = aeronClient->addExclusivePublication(m_publicationURIA, 1);
	auto publicationA = aeronClient->findExclusivePublication(publicationIdA);
	while (!publicationA) {
		std::this_thread::yield();
		publicationA = aeronClient->findExclusivePublication(publicationIdA);
	}

	auto publicationIdB = aeronClient->addExclusivePublication(m_publicationURIB, 1);
	auto publicationB = aeronClient->findExclusivePublication(publicationIdB);
	while (!publicationB) {
		std::this_thread::yield();
		publicationB = aeronClient->findExclusivePublication(publicationIdB);
	}

	auto subscriptionIdA = aeronClient->addSubscription(m_subscriptionURIA, 1);
	auto subscriptionA = aeronClient->findSubscription(subscriptionIdA);
	while (!subscriptionA) {
		std::this_thread::yield();
		subscriptionA = aeronClient->findSubscription(subscriptionIdA);
	}

	auto subscriptionIdB = aeronClient->addSubscription(m_subscriptionURIB, 1);
	auto subscriptionB = aeronClient->findSubscription(subscriptionIdB);
	while (!subscriptionB) {
		std::this_thread::yield();
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

	double bidA = -99, offerA = 99;
	double bidB = -99, offerB = 99;

	/*bool isExpiredA = true, isExpiredB = true;
	time_point<system_clock> timestampA, timestampB, timestampNow;
	timestampA = timestampB = timestampNow = system_clock::now();*/

	std::mt19937_64 randomGenerator(std::random_device{}());

	int ordersCount = 0;
	TriggerDifference currentDiff = DIFF_NONE;
	TriggerDifference currentOrder = DIFF_NONE;

	bool isOrderDelayed = false;
	time_t orderDelay = m_orderDelaySec;
	time_t orderDelayStart = time(nullptr);

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

		auto randomId = randomGenerator();
		tradeData.id(randomId);

		switch (currentOrder) {
			case DIFF_A:
				tradeData.side('2');
				while (publicationA->offer(atomicBuffer, 0, encodedLength) < -1);

				tradeData.side('1');
				while (publicationB->offer(atomicBuffer, 0, encodedLength) < -1);
				break;
			case DIFF_B:
				tradeData.side('1');
				while (publicationA->offer(atomicBuffer, 0, encodedLength) < -1);

				tradeData.side('2');
				while (publicationB->offer(atomicBuffer, 0, encodedLength) < -1);
				break;
		}

		bidA = -99;
		offerA = 99;
		bidB = -99;
		offerB = 99;
		currentOrder = DIFF_NONE;
		orderDelayStart = time(nullptr);
		isOrderDelayed = true;
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

		marketLogger->info("[A] bid: {} offer: {}", bidA, offerA);

		// timestampB = timestampNow;
		// isExpiredA = false;
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

		marketLogger->info("[A] bid: {} offer: {}", bidB, offerB);

		// timestampB = timestampNow;
		// isExpiredB = false;
	};

	auto fragmentAssemblerA = aeron::FragmentAssembler(fragmentHandlerA);
	auto fragmentAssemblerB = aeron::FragmentAssembler(fragmentHandlerB);

	while (m_running) {
		/*timestampNow = system_clock::now();*/

		subscriptionA->poll(fragmentAssemblerA.handler(), 1);
		subscriptionB->poll(fragmentAssemblerB.handler(), 1);

		handleTriggers();

		// TODO: Price expired feature under development
		/*if (!isExpiredA && duration_cast<milliseconds>(timestampNow - timestampA).count() > m_windowMs) {
			bidA = -99;
			offerA = 99;
			isExpiredA = true;
		}

		if (!isExpiredB && duration_cast<milliseconds>(timestampNow - timestampB).count() > m_windowMs) {
			bidB = -99;
			offerB = 99;
			isExpiredB = true;
		}*/
	}
}
