
#include "Messenger.h"

using namespace std::chrono;

Messenger::Messenger(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
                     const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                     const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                     Recorder &recorder, MessengerConfig config)
		: m_control_buffer(control_buffer), m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer),
		  m_recorder(&recorder), m_config(config) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void Messenger::start() {
	m_media_driver = std::thread(&Messenger::mediaDriver, this);
	cpu_set_t cpuset_media_driver;
	CPU_ZERO(&cpuset_media_driver);
	CPU_SET(5, &cpuset_media_driver);
	pthread_setaffinity_np(m_media_driver.native_handle(), sizeof(cpu_set_t), &cpuset_media_driver);
	pthread_setname_np(m_media_driver.native_handle(), "media-driver");
	m_media_driver.detach();

	m_aeron_context.newSubscriptionHandler([](const std::string &channel, std::int32_t streamId,
	                                          std::int64_t correlationId) {
	});

	m_aeron_context.newPublicationHandler([](const std::string &channel, std::int32_t streamId,
	                                         std::int32_t sessionId, std::int64_t correlationId) {
	});

	m_aeron_context.availableImageHandler([&](aeron::Image &image) {
		m_recorder->recordSystemMessage("Messenger: Image available", SYSTEM_RECORD_TYPE_SUCCESS);
		fprintf(stdout, "Messenger: Image available\n");
	});

	m_aeron_context.unavailableImageHandler([&](aeron::Image &image) {
		auto now_us = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
		auto next = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next] = (RemoteMarketDataEvent) {
				.bid = -99.0,
				.offer = 99.0,
				.timestamp_us  = now_us,
				.rec_timestamp_us = now_us
		};
		m_remote_md_buffer->publish(next);
		m_recorder->recordSystemMessage("Messenger: Image unavailable", SYSTEM_RECORD_TYPE_ERROR);
		fprintf(stderr, "Messenger: Image unavailable\n");
	});

	m_aeron_context.errorHandler([&](const std::exception &exception) {
		std::string buffer("Messenger: ");
		buffer.append(exception.what());
		m_recorder->recordSystemMessage(buffer.c_str(), SYSTEM_RECORD_TYPE_ERROR);
		fprintf(stderr, "Messenger: %s\n", exception.what());
	});

	m_aeron_client = std::make_shared<aeron::Aeron>(m_aeron_context);

	std::int64_t md_pub_id = m_aeron_client->addExclusivePublication(m_config.pub_channel,
	                                                                 m_config.market_data_stream_id);
	do {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		m_market_data_ex_pub = m_aeron_client->findExclusivePublication(md_pub_id);
	} while (!m_market_data_ex_pub);

	std::int64_t md_sub_id = m_aeron_client->addSubscription(m_config.sub_channel, m_config.market_data_stream_id);

	do {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		m_market_data_sub = m_aeron_client->findSubscription(md_sub_id);
	} while (!m_market_data_sub);

	cpu_set_t cpuset_poller;
	CPU_ZERO(&cpuset_poller);
	CPU_SET(4, &cpuset_poller);
	m_poller = std::thread(&Messenger::poll, this);
	pthread_setaffinity_np(m_poller.native_handle(), sizeof(cpu_set_t), &cpuset_poller);
	pthread_setname_np(m_poller.native_handle(), "messenger");
	m_poller.detach();
}

void Messenger::mediaDriver() {
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

	while (true) {
		aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
	}

	cleanup:
	aeron_driver_close(driver);
	aeron_driver_context_close(context);
}

void Messenger::poll() {
	bool is_paused = false;
	sbe::MessageHeader sbe_header;
	sbe::MarketData sbe_market_data;

	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE: {
				is_paused = true;
				sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
						.blockLength(sbe::MarketData::sbeBlockLength())
						.templateId(sbe::MarketData::sbeTemplateId())
						.schemaId(sbe::MarketData::sbeSchemaId())
						.version(sbe::MarketData::sbeSchemaVersion());
				sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
				                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
						.bid(-99)
						.offer(99)
						.timestamp(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
				aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();

				while (m_market_data_ex_pub->offer(m_atomic_buffer, 0, len) < -1);
			}
				break;

			case CET_RESUME:
				is_paused = false;
				break;

			default:
				break;
		}

		return false;
	};

	auto local_md_poller = m_local_md_buffer->newPoller();
	m_local_md_buffer->addGatingSequences({local_md_poller->sequence()});
	auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		if (is_paused) {
			return false;
		}

		sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
				.blockLength(sbe::MarketData::sbeBlockLength())
				.templateId(sbe::MarketData::sbeTemplateId())
				.schemaId(sbe::MarketData::sbeSchemaId())
				.version(sbe::MarketData::sbeSchemaVersion());
		sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
		                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
				.bid(data.bid)
				.offer(data.offer)
				.timestamp(data.timestamp_us);

		aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();

		while (m_market_data_ex_pub->offer(m_atomic_buffer, 0, len) < -1);

		return false;
	};

	aeron::NoOpIdleStrategy noOpIdleStrategy;
	aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
	                                               aeron::index_t length, const aeron::Header &header) {
		if (is_paused) {
			return;
		}

		sbe_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSENGER_BUFFER_SIZE);
		sbe_market_data.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
		                              sbe::MessageHeader::encodedLength(), sbe_header.blockLength(),
		                              sbe_header.version(), MESSENGER_BUFFER_SIZE);

		auto next = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next] = (RemoteMarketDataEvent) {
				.bid = sbe_market_data.bid(),
				.offer = sbe_market_data.offer(),
				.timestamp_us  = sbe_market_data.timestamp(),
				.rec_timestamp_us = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count()
		};
		m_remote_md_buffer->publish(next);
	});

	while (true) {
		control_poller->poll(control_handler);
		local_md_poller->poll(local_md_handler);
		noOpIdleStrategy.idle(m_market_data_sub->poll(fragmentAssembler.handler(), 10));
	}
}
