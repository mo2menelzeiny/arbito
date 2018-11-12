
#include "MediaDriver.h"

MediaDriver::MediaDriver() = default;

void MediaDriver::start() {
	auto worker = std::thread(&MediaDriver::work, this);
	worker.detach();
}

void MediaDriver::work() {
	aeron_driver_context_t *context = nullptr;
	aeron_driver_t *driver = nullptr;

	if (aeron_driver_context_init(&context) < 0) {
		goto cleanup;
	}

	context->agent_on_start_func = [](void *, const char *role_name) {
		if (!strcmp(getenv("AERON_THREADING_MODE"), "DEDICATED")) {
			if (strcmp(role_name, "conductor") == 0) {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(2, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			}

			if (strcmp(role_name, "sender") == 0) {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(3, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			}

			if (strcmp(role_name, "receiver") == 0) {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(4, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			}
			pthread_setname_np(pthread_self(), role_name);
		}

		if (!strcmp(getenv("AERON_THREADING_MODE"), "SHARED_NETWORK")) {
			if (strcmp(role_name, "conductor") == 0) {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(3, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), role_name);

			}

			if (strcmp(role_name, "[sender, receiver]") == 0) {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(4, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "sender-receiver");
			}
		}
	};

	if (aeron_driver_init(&driver, context) < 0) {
		goto cleanup;
	}

	if (aeron_driver_start(driver, true) < 0) {
		goto cleanup;
	}

	while (true) {
		aeron_driver_main_idle_strategy(driver, aeron_driver_main_do_work(driver));
	}

	cleanup:
	aeron_driver_close(driver);
	aeron_driver_context_close(context);
}
