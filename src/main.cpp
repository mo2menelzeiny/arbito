
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
#include <IBMarketOffice.h>
#include <IBTradeOffice.h>

int main() {
	std::atomic_bool isRunning(true);

	auto consoleLogger = spdlog::stdout_logger_st<spdlog::async_factory_nonblock>("console");
	auto marketLogger = spdlog::daily_logger_st<spdlog::async_factory_nonblock>("system", "log");

	auto marketDataRingBuffer = Disruptor::RingBuffer<MarketEvent>::createMultiProducer(
			[]() { return MarketEvent(); },
			32,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	auto businessRingBuffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
			[]() { return BusinessEvent(); },
			16,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	BusinessOffice businessOffice(
			marketDataRingBuffer,
			businessRingBuffer,
			stoi(getenv("ORDER_DELAY_SEC")),
			stoi(getenv("MAX_ORDERS")),
			stof(getenv("DIFF_OPEN")),
			stof(getenv("DIFF_CLOSE")),
			getenv("MONGO_URI"),
			getenv("MONGO_DB")
	);

	// BROKER A

	FIXMarketOffice marketOfficeA(
			marketDataRingBuffer,
			getenv("BROKER_A"),
			stof(getenv("QTY_A")),
			getenv("MO_A_HOST"),
			stoi(getenv("MO_A_PORT")),
			getenv("MO_A_USERNAME"),
			getenv("MO_A_PASSWORD"),
			getenv("MO_A_SENDER"),
			getenv("MO_A_TARGET"),
			stoi(getenv("HEARTBEAT")),
			false
	);

	FIXTradeOffice tradeOfficeA(
			businessRingBuffer,
			getenv("BROKER_A"),
			stof(getenv("QTY_A")),
			getenv("TO_A_HOST"),
			stoi(getenv("TO_A_PORT")),
			getenv("TO_A_USERNAME"),
			getenv("TO_A_PASSWORD"),
			getenv("TO_A_SENDER"),
			getenv("TO_A_TARGET"),
			stoi(getenv("HEARTBEAT")),
			false,
			getenv("MONGO_URI"),
			getenv("MONGO_DB")
	);

	// BROKER B

	IBMarketOffice marketOfficeB(
			marketDataRingBuffer,
			getenv("BROKER_B"),
			stof(getenv("QTY_B"))
	);

	FIXTradeOffice tradeOfficeB(
			businessRingBuffer,
			getenv("BROKER_B"),
			stof(getenv("QTY_B")),
			getenv("TO_B_HOST"),
			stoi(getenv("TO_B_PORT")),
			getenv("TO_B_USERNAME"),
			getenv("TO_B_PASSWORD"),
			getenv("TO_B_SENDER"),
			getenv("TO_B_TARGET"),
			stoi(getenv("HEARTBEAT")),
			false,
			getenv("MONGO_URI"),
			getenv("MONGO_DB")
	);

	try {

		while (true) {
			auto businessThread = std::thread([&] {
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(1, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
				pthread_setname_np(pthread_self(), "arbito-business");

				while (isRunning) {
					businessOffice.doWork();
				}

				businessOffice.terminate();
			});

			auto tradeThreadA = std::thread([&] {
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
			});

			auto tradeThreadB = std::thread([&] {
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
			});

			auto marketThreadA = std::thread([&] {
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
			});

			auto marketThreadB = std::thread([&] {
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
			});

			while (isRunning) {
				std::this_thread::sleep_for(milliseconds(10));
			}

			businessThread.join();
			tradeThreadA.join();
			tradeThreadB.join();
			marketThreadA.join();
			marketThreadB.join();

			std::this_thread::sleep_for(seconds(30));
		}

	} catch (std::exception &ex) {
		consoleLogger->error("MAIN {}", ex.what());
	}

	return EXIT_FAILURE;
}