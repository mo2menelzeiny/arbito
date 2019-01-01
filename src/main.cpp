
//Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/BusySpinWaitStrategy.h"

//SPDLOG
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"

// Domain
#include "BusinessOffice.h"
#include "FIXMarketOffice.h"
#include "FIXTradeOffice.h"
#include "IBMarketOffice.h"
#include "IBTradeOffice.h"

int main() {
	auto consoleLogger = spdlog::create_async_nb<spdlog::sinks::stdout_sink_mt>("console");
	auto marketLogger = spdlog::daily_logger_mt<spdlog::async_factory_nonblock>("system", "log");

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

	try {

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

		FIXMarketOffice fixMarketOffice(
				marketDataRingBuffer,
				2,
				getenv("BROKER_A"),
				stof(getenv("QTY_A")),
				getenv("MO_A_HOST"),
				stoi(getenv("MO_A_PORT")),
				getenv("MO_A_USERNAME"),
				getenv("MO_A_PASSWORD"),
				getenv("MO_A_SENDER"),
				getenv("MO_A_TARGET"),
				stoi(getenv("HEARTBEAT"))
		);

		FIXTradeOffice fixTradeOffice(
				businessRingBuffer,
				3,
				getenv("BROKER_A"),
				stof(getenv("QTY_A")),
				getenv("TO_A_HOST"),
				stoi(getenv("TO_A_PORT")),
				getenv("TO_A_USERNAME"),
				getenv("TO_A_PASSWORD"),
				getenv("TO_A_SENDER"),
				getenv("TO_A_TARGET"),
				stoi(getenv("HEARTBEAT")),
				getenv("MONGO_URI"),
				getenv("MONGO_DB")
		);

		// BROKER B

		IBMarketOffice ibMarketOffice(
				marketDataRingBuffer,
				4,
				getenv("BROKER_B"),
				stof(getenv("QTY_B"))
		);

		IBTradeOffice ibTradeOffice(
				businessRingBuffer,
				5,
				getenv("BROKER_B"),
				stof(getenv("QTY_B")),
				getenv("MONGO_URI"),
				getenv("MONGO_DB")
		);

		FIXTradeOffice ibFixTradeOffice(
				businessRingBuffer,
				5,
				getenv("BROKER_B"),
				stof(getenv("QTY_B")),
				getenv("TO_B_HOST"),
				stoi(getenv("TO_B_PORT")),
				getenv("TO_B_USERNAME"),
				getenv("TO_B_PASSWORD"),
				getenv("TO_B_SENDER"),
				getenv("TO_B_TARGET"),
				stoi(getenv("HEARTBEAT")),
				getenv("MONGO_URI"),
				getenv("MONGO_DB")
		);

		 businessOffice.start();
		 fixMarketOffice.start();
		 fixTradeOffice.start();
		 ibMarketOffice.start();
		// ibFixTradeOffice.start();
		 ibTradeOffice.start();

		consoleLogger->info("Main OK");

		while (true) {
			std::this_thread::sleep_for(std::chrono::minutes(1));
		}

	} catch (std::exception &ex) {
		consoleLogger->error("Main {}", ex.what());
	}

	return EXIT_FAILURE;
}