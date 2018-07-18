
#include "CommunicationOffice.h"

CommunicationOffice::CommunicationOffice(const char *pub_channel, int pub_stream_id, const char *sub_channel,
                                         int sub_stream_id)
		: m_aeron_config({pub_channel, pub_stream_id, sub_channel, sub_stream_id}) {

	// disruptor
	m_taskscheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
	m_disruptor = std::make_shared<Disruptor::disruptor<Event>>(
			[]() { return Event(); },
			1024,
			m_taskscheduler,
			Disruptor::ProducerType::Single,
			std::make_shared<Disruptor::BusySpinWaitStrategy>());
}

void CommunicationOffice::mediaDriver() {
	aeron_driver_context_t *context = nullptr;
	aeron_driver_t *driver = nullptr;

	if (aeron_driver_context_init(&context) < 0) {
		fprintf(stderr, "ERROR: context init (%d) %s\n", aeron_errcode(), aeron_errmsg());
		goto cleanup;
	}

	if (aeron_driver_init(&driver, context) < 0) {
		fprintf(stderr, "ERROR: driver init (%d) %s\n", aeron_errcode(), aeron_errmsg());
		goto cleanup;
	}

	if (aeron_driver_start(driver, true) < 0) {
		fprintf(stderr, "ERROR: driver start (%d) %s\n", aeron_errcode(), aeron_errmsg());
		goto cleanup;
	}

	loop:
	aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
	goto loop;

	cleanup:
	aeron_driver_close(driver);
	aeron_driver_context_close(context);
}

void CommunicationOffice::initialize() {
	// Aeron media driver thread
	m_aeron_media_driver = std::thread(&CommunicationOffice::mediaDriver, this);
	m_aeron_media_driver.detach();

	// Aeron configurations
	m_aeron_context.newSubscriptionHandler([](const std::string &channel, std::int32_t streamId,
	                                          std::int64_t correlationId) {
		std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
	});

	m_aeron_context.newPublicationHandler([](const std::string &channel, std::int32_t streamId,
	                                         std::int32_t sessionId, std::int64_t correlationId) {
		std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":"
		          << sessionId << std::endl;
	});

	m_aeron_context.availableImageHandler([&](aeron::Image &image) {
		std::cout << "Available image correlationId=" << image.correlationId() << " sessionId="
		          << image.sessionId();
		std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
	});

	m_aeron_context.unavailableImageHandler([](aeron::Image &image) {
		std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
		          << image.sessionId();
		std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
	});

	m_aeron = std::make_shared<aeron::Aeron>(m_aeron_context);

	std::int64_t publication_id = m_aeron->addPublication(m_aeron_config.pub_channel, m_aeron_config.pub_stream_id);
	std::int64_t subscription_id = m_aeron->addSubscription(m_aeron_config.sub_channel,
	                                                        m_aeron_config.sub_stream_id);

	m_publication = m_aeron->findPublication(publication_id);
	while (!m_publication) {
		std::this_thread::yield();
		m_publication = m_aeron->findPublication(publication_id);
	}
	printf("Publication found!\n");

	m_subscription = m_aeron->findSubscription(subscription_id);
	while (!m_subscription) {
		std::this_thread::yield();
		m_subscription = m_aeron->findSubscription(subscription_id);
	}
	printf("Subscription found!\n");

	// Polling thread
	m_polling_thread = std::thread(&CommunicationOffice::poll, this);
	m_polling_thread.detach();
}

void CommunicationOffice::poll() {
	aeron::BusySpinIdleStrategy idleStrategy;
	aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
	                                               aeron::index_t length, const aeron::Header &header) {
		// TODO: handle reject and confirm messages
		sbe::MessageHeader msgHeader;
		msgHeader.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, AERON_BUFFER_SIZE);

		switch (msgHeader.templateId()) {
			case sbe::MarketData::sbeTemplateId(): {
				sbe::MarketData marketData;
				marketData.wrapForDecode(reinterpret_cast<char *>(buffer.buffer()), msgHeader.encodedLength(),
				                         msgHeader.blockLength(), msgHeader.version(), AERON_BUFFER_SIZE);

				auto next_sequence = m_disruptor->ringBuffer()->next();
				(*m_disruptor->ringBuffer())[next_sequence] = (Event) {
						.event_type = MARKET_DATA,
						.broker = BROKER_SWISSQUOTE,
						.bid = marketData.bid(),
						.bid_qty = marketData.bidQty(),
						.offer = marketData.offer(),
						.offer_qty = marketData.offerQty()
				};
				m_disruptor->ringBuffer()->publish(next_sequence);
			}
				break;
			default:
				fprintf(stderr, "Communication Office: Unknown message template ID\n");
				break;
		}
	});

	while (true) {
		idleStrategy.idle(m_subscription->poll(fragmentAssembler.handler(), 10));
	}
}

void CommunicationOffice::start() {
	m_taskscheduler->start();
	m_disruptor->start();
	initialize();
}

void CommunicationOffice::onEvent(Event &data, std::int64_t sequence, bool endOfBatch) {
	// Only for debugging prices
	/*printf("Communication Office: New market data\n");
	printf("Communication Office: [Bid-Size] %lf \n", data.bid_qty);
	printf("Communication Office: [Bid-Px] %lf \n", data.bid);
	printf("Communication Office: [Bid-Size] %lf \n", data.bid_qty);
	printf("Communication Office: [Offer-Px] %lf \n", data.offer);
	printf("Communication Office: [offer-Size] %lf \n", data.offer_qty);*/

	//NOTE: Communication office will always receive market data event only
	sbe::MessageHeader msgHeader;
	msgHeader.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, AERON_BUFFER_SIZE)
			.blockLength(sbe::MarketData::sbeBlockLength())
			.templateId(sbe::MarketData::sbeTemplateId())
			.schemaId(sbe::MarketData::sbeSchemaId())
			.version(sbe::MarketData::sbeSchemaVersion());

	sbe::MarketData marketData;
	marketData.wrapForEncode(reinterpret_cast<char *>(m_buffer), msgHeader.encodedLength(), AERON_BUFFER_SIZE)
			.bid(data.bid)
			.bidQty(data.bid_qty)
			.offer(data.offer)
			.offerQty(data.offer);

	aeron::index_t len = msgHeader.encodedLength() + marketData.encodedLength();
	aeron::concurrent::AtomicBuffer srcBuffer(m_buffer, AERON_BUFFER_SIZE);
	srcBuffer.putBytes(0, reinterpret_cast<const uint8_t *>(m_buffer), len);
	std::int64_t result;
	do {
		result = m_publication->offer(srcBuffer, 0, len);
	} while (result < 0L);
}


