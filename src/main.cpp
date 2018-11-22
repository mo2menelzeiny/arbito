
//SPDLOG
#include "spdlog/async.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

// Domain
#include "MediaDriver.h"
#include "CentralOffice.h"
#include "FIXMarketOffice.h"
#include "FIXTradeOffice.h"
#include "IBOffice.h"

int main() {
	auto systemLogger = spdlog::create_async_nb<spdlog::sinks::stdout_sink_mt>("system");

	const char *dbUri = getenv("MONGO_URI");
	const char *dbName = getenv("MONGO_DB");

	try {
		auto mediaDriver = MediaDriver();
		mediaDriver.start();

		CentralOffice *centralOffice;
		FIXMarketOffice *fixMarketOffice;
		FIXTradeOffice *fixTradeOffice;
		IBOffice *ibOffice;

		auto arbito = getenv("ARBITO");

		if (!strcmp(arbito, "CENTRAL")) {
			centralOffice = new CentralOffice(
					stoi(getenv("TO_A_PORT")),
					getenv("TO_A_HOST"),
					stoi(getenv("TO_B_PORT")),
					getenv("TO_B_HOST"),
					stoi(getenv("MO_A_PORT")),
					stoi(getenv("MO_B_PORT")),
					stoi(getenv("WINDOW_MS")),
					stoi(getenv("ORDER_DELAY_SEC")),
					stoi(getenv("MAX_ORDERS")),
					stof(getenv("DIFF_OPEN")),
					stof(getenv("DIFF_CLOSE")),
					dbUri,
					dbName
			);

			centralOffice->start();
		}

		if (!strcmp(arbito, "FIX")) {
			fixMarketOffice = new FIXMarketOffice(
					getenv("BROKER"),
					stof(getenv("QTY")),
					stoi(getenv("MO_CO_PORT")),
					getenv("CO_HOST"),
					getenv("MO_HOST"),
					stoi(getenv("MO_PORT")),
					getenv("MO_USERNAME"),
					getenv("MO_PASSWORD"),
					getenv("MO_SENDER"),
					getenv("MO_RECEIVER"),
					stoi(getenv("HEARTBEAT"))
			);

			fixTradeOffice = new FIXTradeOffice(
					getenv("BROKER"),
					stof(getenv("QTY")),
					stoi(getenv("TO_CO_PORT")),
					getenv("TO_HOST"),
					stoi(getenv("TO_PORT")),
					getenv("TO_USERNAME"),
					getenv("TO_PASSWORD"),
					getenv("TO_SENDER"),
					getenv("TO_TARGET"),
					stoi(getenv("HEARTBEAT")),
					dbUri,
					dbName
			);

			fixMarketOffice->start();
			fixTradeOffice->start();
		}


		if (!strcmp(arbito, "IBAPI")) {
			ibOffice = new IBOffice(
					getenv("BROKER"),
					stof(getenv("QTY")),
					stoi(getenv("MO_CO_PORT")),
					getenv("CO_HOST"),
					stoi(getenv("TO_CO_PORT")),
					dbUri,
					dbName
			);

			ibOffice->start();
		}

		systemLogger->info("Main OK");

		while (true) {
			std::this_thread::sleep_for(std::chrono::minutes(1));
		}

	} catch (std::exception &ex) {
		systemLogger->error("Main {}", ex.what());
	}

	return EXIT_FAILURE;
}