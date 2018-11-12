
#include "IBOffice.h"

IBOffice::IBOffice(
		const char *broker,
		double spread,
		double quantity,
		int publicationPort,
		const char *publicationHost,
		int subscriptionPort
) : m_broker(broker),
    m_spread(spread),
    m_quantity(quantity) {
	sprintf(m_publicationURI, "aeron:udp?endpoint=%s:%i\n", publicationHost, publicationPort);
	sprintf(m_subscriptionURI, "aeron:udp?endpoint=0.0.0.0:%i\n", subscriptionPort);
}

void IBOffice::start() {
	auto worker = std::thread(&IBOffice::work, this);
	worker.detach();
}

void IBOffice::work() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "ib-office");

	double bid = 0, offer = 0;
	double bidQty = 0, offerQty = 0;

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

	auto subscriptionId = client->addSubscription(m_subscriptionURI, 1);
	auto subscription = client->findSubscription(subscriptionId);
	while (!subscription) {
		std::this_thread::yield();
		subscription = client->findSubscription(subscriptionId);
	}

	uint8_t buffer[64];
	aeron::concurrent::AtomicBuffer atomicBuffer;
	atomicBuffer.wrap(buffer, 64);

	auto marketDataMessageBuffer = reinterpret_cast<char *>(buffer);

	sbe::MessageHeader marketDataHeader;
	sbe::MarketData marketData;

	marketDataHeader
			.wrap(marketDataMessageBuffer, 0, 0, 64)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	marketData.wrapForEncode(marketDataMessageBuffer, sbe::MessageHeader::encodedLength(), 64);

	auto encodedLength = static_cast<aeron::index_t>(sbe::MessageHeader::encodedLength() + marketData.encodedLength());

	auto onTickHandler = OnTickHandler([&](TickType tickType, double value) {
		switch (tickType) {
			case ASK:
				offer = value;
				break;
			case BID:
				bid = value;
				break;
			case BID_SIZE:
				bidQty = value;
			case ASK_SIZE:
				offerQty = value;
				break;
			default:
				break;
		}

		if (m_spread < offer - bid || m_quantity > offerQty || m_quantity > bidQty) return;

		marketData
				.bid(bid)
				.offer(offer);

		while (publication->offer(atomicBuffer, 0, encodedLength) < -1);
	});

	sbe::MessageHeader tradeDataHeader;
	sbe::TradeData tradeData;

	bool canOrder = false;

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

		canOrder = true;
	};

	auto fragmentAssembler = aeron::FragmentAssembler(fragmentHandler);

	auto stateHandler = StateHandler([&](State *state) {
		if (!canOrder) return;

		char clOrdId[64];
		sprintf(clOrdId, "%li", tradeData.id());

		// TODO: handle IB orders
		switch (tradeData.side()) {
			case '1': // SELL

				break;
			case '2': // BUY

				break;
			default:
				break;
		}

		canOrder = false;
	});

	IBClient ibClient(onTickHandler, stateHandler);

	while (true) {
		ibClient.connect("127.0.0.1", 4001);

		while (ibClient.isConnected()) {
			ibClient.processMessages();
			subscription->poll(fragmentAssembler.handler(), 1);
		}
	}

}
