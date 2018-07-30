
#include <Messenger.h>
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"

int main() {

	try {
		// Getting parameters from environment variables
		double spread = atof(getenv("SPREAD"));
		double bid_lot_size = atof(getenv("BID_LOT_SIZE")), offer_lot_size = atof(getenv("OFFER_LOT_SIZE"));
		// double diff_open = atof(getenv("DIFF_OPEN")), diff_close = atof(getenv("DIFF_CLOSE"));
		int pub_stream_id = atoi(getenv("PUB_STREAM_ID")), sub_stream_id = atoi(getenv("SUB_STREAM_ID"));
		const char *pub_channel = getenv("PUB_CHANNEL"), *sub_channel = getenv("SUB_CHANNEL");
		const char *mo_host = "fix-marketdata.london-demo.lmax.com", *mo_username = "AhmedDEMO",
				*mo_password = "password1", *mo_sender = "AhmedDEMO", *mo_receiver = "LMXBDM";
		const char *to_host = "fix-order.london-demo.lmax.com", *to_username = "AhmedDEMO",
				*to_password = "password1", *to_sender = "AhmedDEMO", *to_receiver = "LMXBD";
		int port = 443, heartbeat = 15;

		std::shared_ptr<Messenger> messenger = std::make_shared<Messenger>();

		std::shared_ptr<LMAX::MarketOffice> market_office =
				std::make_shared<LMAX::MarketOffice>(mo_host, port, mo_username, mo_password, mo_sender, mo_receiver,
				                                     heartbeat, spread, bid_lot_size, offer_lot_size);

		/*std::shared_ptr<LMAX::TradeOffice> trade_office =
				std::make_shared<LMAX::TradeOffice>(to_host, port, to_username, to_password, to_sender, to_receiver,
				                                    heartbeat, pub_channel, pub_stream_id, sub_channel, sub_stream_id,
				                                    diff_open, diff_close);
*/
		//market_office->addConsumer<LMAX::TradeOffice>(trade_office);
		market_office->start();
		//trade_office->start();

		while (true) {
			std::this_thread::yield();
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << " : " << std::endl;
	}
	return EXIT_FAILURE;
}