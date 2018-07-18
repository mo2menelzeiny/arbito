
#include <BusinessOffice.h>
#include "MarketDataOffice.h"
#include "CommunicationOffice.h"

int main() {

	try {
		std::shared_ptr<BusinessOffice> business_office = std::make_shared<BusinessOffice>(10, 10, 10);

		const char *pub_channel = getenv("PUB_CHANNEL"), *sub_channel = getenv("SUB_CHANNEL");
		int pub_stream_id = atoi(getenv("PUB_STREAM_ID")), sub_stream_id = atoi(getenv("SUB_STREAM_ID"));

		std::shared_ptr<CommunicationOffice> com_office =
				std::make_shared<CommunicationOffice>(pub_channel, pub_stream_id, sub_channel, sub_stream_id);

		com_office->addConsumer(business_office);
		com_office->start();

		const char *host = "fix-marketdata.london-demo.lmax.com", *username = "AhmedDEMO", *password = "password1",
				*sender = "AhmedDEMO", *receiver = "LMXBDM";
		int port = 443, heartbeat = 15;

		std::shared_ptr<LMAX::MarketDataOffice> md_office =
				std::make_shared<LMAX::MarketDataOffice>(host, port, username, password, sender, receiver, heartbeat);

		md_office->addConsumer(com_office);
		md_office->addConsumer(business_office);
		md_office->start();

		while (true) {
			std::this_thread::yield();
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << " : " << std::endl;
	}
	return EXIT_FAILURE;
}