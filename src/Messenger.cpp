
#include "Messenger.h"

Messenger::Messenger(Recorder &recorder) : m_recorder(&recorder) {}

const std::shared_ptr<aeron::Aeron> &Messenger::aeronClient() const {
	return m_aeron_client;
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

void Messenger::start() {
	// Aeron media driver thread
	m_media_driver = std::thread(&Messenger::mediaDriver, this);
	m_media_driver.detach();

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
		m_recorder->recordSystem("Messenger: Image OK", SYSTEM_RECORD_TYPE_SUCCESS);
	});

	m_aeron_context.unavailableImageHandler([&](aeron::Image &image) {
		std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId="
		          << image.sessionId();
		std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
		m_recorder->recordSystem("Messenger: Image FAILED", SYSTEM_RECORD_TYPE_ERROR);
	});

	m_aeron_context.errorHandler([&](const std::exception &exception) {
		fprintf(stderr, "Messenger: %s\n", exception.what());
		std::string buffer("Messenger: ");
		buffer.append(exception.what());
		m_recorder->recordSystem(buffer.c_str(), SYSTEM_RECORD_TYPE_ERROR);
	});

	m_aeron_client = std::make_shared<aeron::Aeron>(m_aeron_context);
}
