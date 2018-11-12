
// Domain
#include <MediaDriver.h>
#include <CentralOffice.h>
#include <FIXMarketOffice.h>
#include <FIXTradeOffice.h>
#include <IBOffice.h>

int main() {
	try {
		pthread_setname_np(pthread_self(), "main");
		srand(static_cast<unsigned int>(time(nullptr)));

		const char *uri_string = getenv("MONGO_URI");
		const char *db_name = getenv("MONGO_DB");

		auto mediaDriver = MediaDriver();
		mediaDriver.start();

		CentralOffice *centralOffice;
		FIXMarketOffice *fixMarketOffice;
		FIXTradeOffice *fixTradeOffice;
		IBOffice *ibOffice;

		auto nodeType = getenv("NODE_TYPE");

		if (!strcmp(nodeType, "CENTRAL")) {
			centralOffice = new CentralOffice(
					stoi(getenv("TO_A_PORT")),
					getenv("TO_A_HOST"),
					stoi(getenv("TO_B_PORT")),
					getenv("TO_B_HOST"),
					stoi(getenv("MO_A_PORT")),
					stoi(getenv("MO_B_PORT")),
					stoi(getenv("WINDOW_MS")),
					stoi(getenv("MAX_ORDERS")),
					stoi(getenv("DIFF_OPEN")),
					stoi(getenv("DIFF_CLOSE"))
			);
			centralOffice->start();
		}

		if (!strcmp(nodeType, "FIX")) {
			fixMarketOffice = new FIXMarketOffice(
					getenv("BROKER"),
					stof(getenv("SPREAD")),
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
					stoi(getenv("QTY")),
					stoi(getenv("TO_CO_PORT")),
					getenv("TO_HOST"),
					stoi(getenv("TO_PORT")),
					getenv("TO_USERNAME"),
					getenv("TO_PASSWORD"),
					getenv("TO_SENDER"),
					getenv("TO_TARGET"),
					stoi(getenv("HEARTBEAT"))
			);

			// TODO: disable trade office
			fixMarketOffice->start();
			// fixTradeOffice->start();
		}


		if (!strcmp(nodeType, "IB")) {
			ibOffice = new IBOffice(
					getenv("BROKER"),
					stof(getenv("SPREAD")),
					stof(getenv("QTY")),
					stoi(getenv("MO_CO_PORT")),
					getenv("CO_HOST"),
					stoi(getenv("TO_CO_PORT"))
			);

			ibOffice->start();
		}

		while (true) {
			std::this_thread::sleep_for(std::chrono::minutes(1));
		}
	} catch (std::exception &ex) {
		printf("EXCEPTION: %s\n", ex.what());
	}
}