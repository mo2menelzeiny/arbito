
#include "Messenger.h"

Messenger::Messenger(
		const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
		const shared_ptr<RingBuffer<MarketDataEvent>> &local_md_buffer,
		const shared_ptr<RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
		Recorder &recorder,
		MessengerConfig config
) : m_control_buffer(control_buffer),
    m_local_md_buffer(local_md_buffer),
    m_remote_md_buffer(remote_md_buffer),
    m_recorder(&recorder),
    m_config(config) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void Messenger::start() {
	auto media_driver = thread(&Messenger::mediaDriver, this);
	media_driver.detach();

	m_aeron_context.availableImageHandler([&](Image &image) {
		try {
			auto next_resume = m_control_buffer->tryNext();
			(*m_control_buffer)[next_resume] = (ControlEvent) {
					.type = CET_RESUME,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_resume);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent("Messenger: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}
		m_recorder->systemEvent("Messenger: Node available", SE_TYPE_SUCCESS);
	});

	m_aeron_context.unavailableImageHandler([&](Image &image) {
		try {
			auto next_pause = m_control_buffer->tryNext();
			(*m_control_buffer)[next_pause] = (ControlEvent) {
					.type = CET_PAUSE,
					.timestamp_ms  = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
			};
			m_control_buffer->publish(next_pause);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent("Messenger: control buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}
		m_recorder->systemEvent("Messenger: Node unavailable", SE_TYPE_ERROR);
	});

	m_aeron_context.errorHandler([&](const exception &exception) {
		string buffer("Messenger: ");
		buffer.append(exception.what());
		m_recorder->systemEvent(buffer.c_str(), SE_TYPE_ERROR);
	});

	m_aeron_client = make_shared<Aeron>(m_aeron_context);

	int64_t md_pub_id = m_aeron_client->addExclusivePublication(
			m_config.pub_channel,
			m_config.md_stream_id
	);

	do {
		this_thread::sleep_for(chrono::nanoseconds(1));
		m_market_data_ex_pub = m_aeron_client->findExclusivePublication(md_pub_id);
	} while (!m_market_data_ex_pub);

	int64_t md_sub_id = m_aeron_client->addSubscription(m_config.sub_channel, m_config.md_stream_id);

	do {
		this_thread::sleep_for(chrono::nanoseconds(1));
		m_market_data_sub = m_aeron_client->findSubscription(md_sub_id);
	} while (!m_market_data_sub);

	auto poller = thread(&Messenger::poll, this);
	poller.detach();
}

void Messenger::mediaDriver() {
	aeron_driver_context_t *context = nullptr;
	aeron_driver_t *driver = nullptr;

	if (aeron_driver_context_init(&context) < 0) {
		m_recorder->systemEvent("Messenger: context init", SE_TYPE_ERROR);
		goto cleanup;
	}

	context->agent_on_start_func = [](void *, const char *role_name) {
		if (strcmp(role_name, "conductor") == 0) {
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(5, &cpuset);
			pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

		} else if (strcmp(role_name, "sender") == 0) {
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(6, &cpuset);
			pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

		} else if (strcmp(role_name, "receiver") == 0) {
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(7, &cpuset);
			pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
		}

		pthread_setname_np(pthread_self(), role_name);
	};

	if (aeron_driver_init(&driver, context) < 0) {
		m_recorder->systemEvent("Messenger: Driver init", SE_TYPE_ERROR);
		goto cleanup;
	}

	if (aeron_driver_start(driver, true) < 0) {
		m_recorder->systemEvent("Messenger: driver start", SE_TYPE_ERROR);
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
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(4, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "messenger");

	bool is_paused = false;
	MessageHeader sbe_header;
	MarketData sbe_market_data;

	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE: {
				is_paused = true;

				sbe_header
						.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
						.blockLength(MarketData::sbeBlockLength())
						.templateId(MarketData::sbeTemplateId())
						.schemaId(MarketData::sbeSchemaId())
						.version(MarketData::sbeSchemaVersion());

				sbe_market_data
						.wrapForEncode(
								reinterpret_cast<char *>(m_buffer),
								MessageHeader::encodedLength(),
								MESSENGER_BUFFER_SIZE
						)
						.bid(-99)
						.offer(99)
						.timestamp(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());

				index_t len = MessageHeader::encodedLength() + sbe_market_data.encodedLength();

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
	auto local_md_handler = [&](MarketDataEvent &data, int64_t sequence, bool endOfBatch) -> bool {
		if (is_paused) {
			return false;
		}

		sbe_header
				.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
				.blockLength(MarketData::sbeBlockLength())
				.templateId(MarketData::sbeTemplateId())
				.schemaId(MarketData::sbeSchemaId())
				.version(MarketData::sbeSchemaVersion());

		sbe_market_data
				.wrapForEncode(
						reinterpret_cast<char *>(m_buffer),
						MessageHeader::encodedLength(),
						MESSENGER_BUFFER_SIZE
				)
				.bid(data.bid)
				.offer(data.offer)
				.timestamp(data.timestamp_ms);

		index_t len = MessageHeader::encodedLength() + sbe_market_data.encodedLength();

		while (m_market_data_ex_pub->offer(m_atomic_buffer, 0, len) < -1);

		return false;
	};

	auto fragmentAssemblerHandler = [&](AtomicBuffer &buffer, index_t offset, index_t length, const Header &header) {
		if (is_paused) {
			return;
		}

		sbe_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSENGER_BUFFER_SIZE);

		sbe_market_data.wrapForDecode(
				reinterpret_cast<char *>(buffer.buffer() + offset),
				MessageHeader::encodedLength(),
				sbe_header.blockLength(),
				sbe_header.version(),
				MESSENGER_BUFFER_SIZE
		);

		auto data = (RemoteMarketDataEvent) {
				.bid = sbe_market_data.bid(),
				.offer = sbe_market_data.offer(),
				.timestamp_ms  = sbe_market_data.timestamp(),
				.rec_timestamp_ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
		};

		try {
			auto next = m_remote_md_buffer->tryNext();
			(*m_remote_md_buffer)[next] = data;
			m_remote_md_buffer->publish(next);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent("Messenger: remote buffer InsufficientCapacityException", SE_TYPE_ERROR);
		}

		try {
			auto next_record = m_recorder->m_remote_records_buffer->tryNext();
			(*m_recorder->m_remote_records_buffer)[next_record] = data;
			m_recorder->m_remote_records_buffer->publish(next_record);
		} catch (InsufficientCapacityException &e) {
			m_recorder->systemEvent(
					"Messenger: remote records buffer InsufficientCapacityException",
					SE_TYPE_ERROR
			);
		}
	};

	FragmentAssembler fragmentAssembler(fragmentAssemblerHandler);
	NoOpIdleStrategy noOpIdleStrategy;

	while (true) {
		control_poller->poll(control_handler);
		local_md_poller->poll(local_md_handler);
		noOpIdleStrategy.idle(m_market_data_sub->poll(fragmentAssembler.handler(), 10));
	}
}
