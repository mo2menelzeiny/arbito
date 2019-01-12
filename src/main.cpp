
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
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "arbito-core");

	std::atomic_bool isRunning(true);

	auto consoleLogger = spdlog::stdout_logger_st<spdlog::async_factory_nonblock>("console");
	auto marketLogger = spdlog::daily_logger_st<spdlog::async_factory_nonblock>("system", "log");

	auto marketDataRingBuffer = Disruptor::RingBuffer<MarketDataEvent>::createMultiProducer(
			[]() { return MarketDataEvent(); },
			8,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	auto businessRingBuffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
			[]() { return BusinessEvent(); },
			8,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	BusinessOffice businessOffice(
			marketDataRingBuffer,
			businessRingBuffer,
			1,
			stoi(getenv("WINDOW_MS")),
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
			true
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
			true,
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

	using namespace std::chrono;
	long counter = 0, duration = 0, max = 0, total = 0;
	time_point<system_clock> start, end;

	try {

		while (isRunning) {
			start = system_clock::now();

			marketOfficeA.doWork();
			// marketOfficeB.doWork();
			tradeOfficeA.doWork();
			// tradeOfficeB.doWork();

			end = system_clock::now();

			duration = duration_cast<nanoseconds>(end - start).count();
			total += duration;
			++counter;

			if (duration > max) {
				max = duration;
			}

			if (counter > 1000000) {
				printf("max: %li avg: %li \n", max, total / counter);
				max = 0;
				counter = 0;
				total = 0;
			}
		}

	} catch (std::exception &ex) {
		consoleLogger->error("{}", ex.what());
	}

	return EXIT_FAILURE;
}