
#include "Messenger.h"

Messenger::Messenger(const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                     Recorder &recorder, MessengerConfig config)
		: m_remote_md_buffer(remote_md_buffer), m_recorder(&recorder), m_config(config) {}

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

void Messenger::start() {
	m_aeron_context.newSubscriptionHandler([](const std::string &channel, std::int32_t streamId,
	                                          std::int64_t correlationId) {
	});

	m_aeron_context.newPublicationHandler([](const std::string &channel, std::int32_t streamId,
	                                         std::int32_t sessionId, std::int64_t correlationId) {
	});

	m_aeron_context.availableImageHandler([&](aeron::Image &image) {
		m_recorder->recordSystem("Messenger: Image available", SYSTEM_RECORD_TYPE_SUCCESS);
		fprintf(stdout, "Messenger: Image available\n");
	});

	m_aeron_context.unavailableImageHandler([&](aeron::Image &image) {
		auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
		auto next = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next] = (RemoteMarketDataEvent) {
				.bid = -99.0,
				.offer = 99.0,
				.timestamp_us  = now_us,
				.rec_timestamp_us = now_us
		};
		m_remote_md_buffer->publish(next);
		m_recorder->recordSystem("Messenger: Image unavailable", SYSTEM_RECORD_TYPE_ERROR);
		fprintf(stderr, "Messenger: Image unavailable\n");
	});

	m_aeron_context.errorHandler([&](const std::exception &exception) {
		std::string buffer("Messenger: ");
		buffer.append(exception.what());
		m_recorder->recordSystem(buffer.c_str(), SYSTEM_RECORD_TYPE_ERROR);
		fprintf(stderr, "Messenger: %s\n", exception.what());
	});

	m_media_driver = std::thread(&Messenger::mediaDriver, this);
	m_media_driver.detach();

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
}

const std::shared_ptr<aeron::Subscription> &Messenger::marketDataSub() const {
	return m_market_data_sub;
}

const std::shared_ptr<aeron::ExclusivePublication> &Messenger::marketDataExPub() const {
	return m_market_data_ex_pub;
}
