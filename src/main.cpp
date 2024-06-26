
//Disruptor
#include <Disruptor/Disruptor.h>
#include <Disruptor/BusySpinWaitStrategy.h>

//SPDLOG
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

// Domain
#include <BusinessOffice.h>
#include <FIXMarketOffice.h>
#include <FIXTradeOffice.h>
#include <StreamOffice.h>
#include <LimitOffice.h>

int main() {
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	std::atomic_bool isRunning(true);

	auto consoleLogger = spdlog::stdout_logger_mt("console");
	auto marketLogger = spdlog::daily_logger_mt("system", "log");

	auto priceRingBuffer = Disruptor::RingBuffer<PriceEvent>::createMultiProducer(
			[]() { return PriceEvent(); },
			32,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	/*auto orderRingBuffer = Disruptor::RingBuffer<OrderEvent>::createMultiProducer(
			[]() { return OrderEvent(); },
			16,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	auto executionRingBuffer = Disruptor::RingBuffer<ExecutionEvent>::createMultiProducer(
			[]() { return ExecutionEvent(); },
			16,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);*/

	StreamOffice streamOffice(
			priceRingBuffer,
			getenv("NATS_HOST"),
			getenv("NATS_USER"),
			getenv("NATS_PASS")
	);

	/*BusinessOffice triggerOffice(
			priceRingBuffer,
			executionRingBuffer,
			orderRingBuffer,
			stoi(getenv("ORDER_DELAY_SEC")),
			stoi(getenv("MAX_ORDERS")),
			stof(getenv("DIFF_OPEN")),
			stof(getenv("DIFF_CLOSE")),
			getenv("MONGO_URI"),
			getenv("MONGO_DB")
	);*/

	// BROKER A

	FIXMarketOffice marketOfficeA(
			priceRingBuffer,
			getenv("BROKER_A"),
			stof(getenv("QTY_A")),
			getenv("MO_A_HOST"),
			stoi(getenv("MO_A_PORT")),
			getenv("MO_A_USERNAME"),
			getenv("MO_A_PASSWORD"),
			getenv("MO_A_SENDER"),
			getenv("MO_A_TARGET"),
			stoi(getenv("HEARTBEAT")),
			strcmp(getenv("SSL_A"), "1") == 0
	);


	/*FIXTradeOffice tradeOfficeA(
			orderRingBuffer,
			executionRingBuffer,
			getenv("BROKER_A"),
			stof(getenv("QTY_A")),
			getenv("TO_A_HOST"),
			stoi(getenv("TO_A_PORT")),
			getenv("TO_A_USERNAME"),
			getenv("TO_A_PASSWORD"),
			getenv("TO_A_SENDER"),
			getenv("TO_A_TARGET"),
			stoi(getenv("HEARTBEAT")),
			true,
			getenv("MONGO_URI"),
			getenv("MONGO_DB")
	);*/

	// BROKER B

	FIXMarketOffice marketOfficeB(
			priceRingBuffer,
			getenv("BROKER_B"),
			stof(getenv("QTY_B")),
			getenv("MO_B_HOST"),
			stoi(getenv("MO_B_PORT")),
			getenv("MO_B_USERNAME"),
			getenv("MO_B_PASSWORD"),
			getenv("MO_B_SENDER"),
			getenv("MO_B_TARGET"),
			stoi(getenv("HEARTBEAT")),
			strcmp(getenv("SSL_B"), "1") == 0
	);

	try {

		while (true) {
			consoleLogger->info("MAIN OK");

			/*auto businessThread = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(1, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-business");

				try {

					triggerOffice.initiate();

					while (isRunning) {
						triggerOffice.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("Business Offices {}", ex.what());
					isRunning = false;
				}

				triggerOffice.terminate();
			});*/

			/*auto tradeThreadA = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(2, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-trade");

				try {

					tradeOfficeA.initiate();

					while (isRunning) {
						tradeOfficeA.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("[{}] Trade Office {}", getenv("BROKER_A"), ex.what());
					isRunning = false;
				}

				tradeOfficeA.terminate();
			});*/

			/*auto marketThreadA = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(4, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-market");

				try {

					marketOfficeA.initiate();

					while (isRunning) {
						marketOfficeA.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("[{}] Market Office {}", getenv("BROKER_A"), ex.what());
					isRunning = false;
				}

				marketOfficeA.terminate();
			});*/

			/*auto tradeThreadB = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(3, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-trade");

				try {

					tradeOfficeB.initiate();

					while (isRunning) {
						tradeOfficeB.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("[{}] Trade Office {}", getenv("BROKER_B"), ex.what());
					isRunning = false;
				}

				tradeOfficeB.terminate();
			});*/

			/*auto marketThreadB = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(5, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-market");

				try {

					marketOfficeB.initiate();

					while (isRunning) {
						marketOfficeB.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("[{}] Market Office {}", getenv("BROKER_B"), ex.what());
					isRunning = false;
				}

				marketOfficeB.terminate();
			});*/

			/*auto streamThread = std::thread([&] {
				pthread_setname_np(pthread_self(), "arbito-stream");

				try {
					streamOffice.initiate();

					while (isRunning) {
						streamOffice.doWork();
					}
				} catch (std::exception &ex) {
					consoleLogger->error("Stream Office {}", ex.what());
					isRunning = false;
				}

				streamOffice.terminate();
			});*/

			auto mainThread = std::thread([&] {
				pthread_setname_np(pthread_self(), "arbito-main");

				try {
					marketOfficeA.initiate();
					marketOfficeB.initiate();
					streamOffice.initiate();

					while (isRunning) {
						marketOfficeA.doWork();
						marketOfficeB.doWork();
						streamOffice.doWork();
					}

				} catch (std::exception &ex) {
					consoleLogger->error("Main Thread {}", ex.what());
					isRunning = false;
				}

				marketOfficeA.terminate();
				marketOfficeB.terminate();
				streamOffice.terminate();
			});

			while (isRunning) {
				std::this_thread::sleep_for(milliseconds(10));
			}

			/*businessThread.join();*/
			/*tradeThreadA.join();*/
			/*tradeThreadB.join();*/
			/*marketThreadA.join();*/
			/*marketThreadB.join();*/
			/*streamThread.join();*/
			mainThread.join();

			std::this_thread::sleep_for(seconds(10));

			consoleLogger->error("MAIN FAILED");
		}

	} catch (std::exception &ex) {
		consoleLogger->error("MAIN {}", ex.what());
	}

	EVP_cleanup();

	return EXIT_FAILURE;
}