
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"
#include "Messenger.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/ThreadPerTaskScheduler.h"
#include "Disruptor/BusySpinWaitStrategy.h"

int main() {

	try {
		double spread = atof(getenv("SPREAD"));
		double offer_lot_size = atof(getenv("OFFER_LOT_SIZE"));
		double bid_lot_size = atof(getenv("BID_LOT_SIZE"));
		double diff_open = atof(getenv("DIFF_OPEN"));
		double diff_close = atof(getenv("DIFF_CLOSE"));

		const char *pub_channel = getenv("PUB_CHANNEL");
		int pub_stream_id = atoi(getenv("PUB_STREAM_ID"));
		const char *sub_channel = getenv("SUB_CHANNEL");
		int sub_stream_id = atoi(getenv("SUB_STREAM_ID"));

		const char *mo_host = getenv("MO_HOST");
		const char *mo_username = getenv("MO_USERNAME");
		const char *mo_password = getenv("MO_PASSWORD");
		const char *mo_sender = getenv("MO_SENDER");
		const char *mo_receiver = getenv("MO_RECEIVER");

		const char *to_host = getenv("TO_HOST");
		const char *to_username = getenv("TO_USERNAME");
		const char *to_password = getenv("TO_PASSWORD");
		const char *to_sender = getenv("TO_SENDER");
		const char *to_receiver = getenv("TO_RECEIVER");

		int port = atoi(getenv("PORT"));
		int heartbeat = atoi(getenv("HEARTBEAT"));

		int broker = atoi(getenv("BROKER"));

		auto task_scheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();

		auto broker_market_data_disruptor = std::make_shared<Disruptor::disruptor<MarketDataEvent>>(
				[]() { return MarketDataEvent(); },
				1024,
				task_scheduler,
				Disruptor::ProducerType::Single,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto arbitrage_data_disruptor = std::make_shared<Disruptor::disruptor<ArbitrageDataEvent>>(
				[]() { return ArbitrageDataEvent(); },
				1024,
				task_scheduler,
				Disruptor::ProducerType::Single,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());

		std::shared_ptr<Messenger> messenger = std::make_shared<Messenger>();

		std::shared_ptr<LMAX::MarketOffice> lmax_market_office;
		std::shared_ptr<LMAX::TradeOffice> lmax_trade_office;

		std::shared_ptr<SWISSQUOTE::MarketOffice> swissquote_market_office;
		std::shared_ptr<SWISSQUOTE::TradeOffice> swissquote_trade_office;

		switch (broker) {
			case 1: {
				fprintf(stdout, "Broker -> LMAX\n");
				lmax_market_office = std::make_shared<LMAX::MarketOffice>(messenger,
				                                                          broker_market_data_disruptor,
				                                                          arbitrage_data_disruptor,
				                                                          mo_host,
				                                                          port,
				                                                          mo_username,
				                                                          mo_password,
				                                                          mo_sender,
				                                                          mo_receiver,
				                                                          heartbeat,
				                                                          pub_channel,
				                                                          pub_stream_id,
				                                                          sub_channel,
				                                                          sub_stream_id,
				                                                          spread,
				                                                          bid_lot_size,
				                                                          offer_lot_size);
				lmax_trade_office = std::make_shared<LMAX::TradeOffice>(messenger,
				                                                        arbitrage_data_disruptor,
				                                                        to_host,
				                                                        port,
				                                                        to_username,
				                                                        to_password,
				                                                        to_sender,
				                                                        to_receiver,
				                                                        heartbeat,
				                                                        diff_open,
				                                                        diff_close,
				                                                        bid_lot_size,
				                                                        offer_lot_size);
			}

				lmax_market_office->start();
				lmax_trade_office->start();
				break;

			case 2: {
				fprintf(stdout, "Broker -> SWISSQUOTE\n");
				swissquote_trade_office = std::make_shared<SWISSQUOTE::TradeOffice>(messenger,
				                                                                    arbitrage_data_disruptor,
				                                                                    to_host,
				                                                                    port,
				                                                                    to_username,
				                                                                    to_password,
				                                                                    to_sender,
				                                                                    to_receiver,
				                                                                    heartbeat,
				                                                                    diff_open,
				                                                                    diff_close,
				                                                                    bid_lot_size,
				                                                                    offer_lot_size);

				swissquote_market_office = std::make_shared<SWISSQUOTE::MarketOffice>(messenger,
				                                                                      broker_market_data_disruptor,
				                                                                      arbitrage_data_disruptor,
				                                                                      mo_host,
				                                                                      port,
				                                                                      mo_username,
				                                                                      mo_password,
				                                                                      mo_sender,
				                                                                      mo_receiver,
				                                                                      heartbeat,
				                                                                      pub_channel,
				                                                                      pub_stream_id,
				                                                                      sub_channel,
				                                                                      sub_stream_id,
				                                                                      spread,
				                                                                      bid_lot_size,
				                                                                      offer_lot_size);
			}
				swissquote_market_office->start();
				swissquote_trade_office->start();
				break;
			default:
				fprintf(stderr, "Main: Undefined broker\n");
		}

		task_scheduler->start();
		broker_market_data_disruptor->start();
		arbitrage_data_disruptor->start();

		while (true) {
			std::this_thread::yield();
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << " : " << std::endl;
	}
	return EXIT_FAILURE;
}