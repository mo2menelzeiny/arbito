
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"
#include "Messenger.h"
#include "Recorder.h"
#include "BrokerMarketDataHandler.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/RingBuffer.h"
#include "Disruptor/RoundRobinThreadAffinedTaskScheduler.h"
#include "Disruptor/BusySpinWaitStrategy.h"
#include "Disruptor/SleepingWaitStrategy.h"

// MongoDB
#include <mongoc.h>

int main() {

	try {
		double spread = getenv("SPREAD") ? atof(getenv("SPREAD")) : 0.0001;
		double lot_size = getenv("LOT_SIZE") ? atof(getenv("LOT_SIZE")) : 1.0;
		double diff_open = getenv("DIFF_OPEN") ? atof(getenv("DIFF_OPEN")) : -1.0;
		double diff_close = getenv("DIFF_CLOSE") ? atof(getenv("DIFF_CLOSE")) : -1.0;
		const char *pub_channel = getenv("PUB_CHANNEL") ? getenv("PUB_CHANNEL") : "aeron:udp?endpoint=localhost:50501";
		int pub_stream_id = getenv("PUB_STREAM_ID") ? atoi(getenv("PUB_STREAM_ID")) : 51;
		const char *sub_channel = getenv("SUB_CHANNEL") ? getenv("SUB_CHANNEL") : "aeron:udp?endpoint=0.0.0.0:50501";
		int sub_stream_id = getenv("SUB_STREAM_ID") ? atoi(getenv("SUB_STREAM_ID")) : 51;
		const char *mo_host = getenv("MO_HOST") ? getenv("MO_HOST") : "fix-marketdata.london-demo.lmax.com";
		const char *mo_username = getenv("MO_USERNAME") ? getenv("MO_USERNAME") : "AhmedDEMO";
		const char *mo_password = getenv("MO_PASSWORD") ? getenv("MO_PASSWORD") : "password1";
		const char *mo_sender = getenv("MO_SENDER") ? getenv("MO_SENDER") : "AhmedDEMO";
		const char *mo_receiver = getenv("MO_RECEIVER") ? getenv("MO_RECEIVER") : "LMXBDM";
		const char *to_host = getenv("TO_HOST") ? getenv("TO_HOST") : "fix-order.london-demo.lmax.com";
		const char *to_username = getenv("TO_USERNAME") ? getenv("TO_USERNAME") : "AhmedDEMO";
		const char *to_password = getenv("TO_PASSWORD") ? getenv("TO_PASSWORD") : "password1";
		const char *to_sender = getenv("TO_SENDER") ? getenv("TO_SENDER") : "AhmedDEMO";
		const char *to_receiver = getenv("TO_RECEIVER") ? getenv("TO_RECEIVER") : "LMXBD";
		int port = getenv("PORT") ? atoi(getenv("PORT")) : 443;
		int heartbeat = getenv("HEARTBEAT") ? atoi(getenv("HEARTBEAT")) : 30;
		int broker = getenv("BROKER") ? atoi(getenv("BROKER")) : 1;
		const char *uri_string = getenv("MONGO_URI")
		                         ? getenv("MONGO_URI")
		                         : "mongodb://arbito:mlab_6852@ds123012.mlab.com:23012/db_arbito_local";
		const char *db_name = getenv("MONGO_DB") ? getenv("MONGO_DB") : "db_arbito_local";

		fprintf(stdout, "Main: System starting..\n");

		auto recorder = std::make_shared<Recorder>(uri_string, broker, db_name);

		recorder->recordSystem("Main: initialize OK", SYSTEM_RECORD_TYPE_SUCCESS);

		auto task_scheduler = std::make_shared<Disruptor::RoundRobinThreadAffinedTaskScheduler>();

		auto broker_market_data_disruptor = std::make_shared<Disruptor::disruptor<MarketDataEvent>>(
				[]() { return MarketDataEvent(); },
				16,
				task_scheduler,
				Disruptor::ProducerType::Single,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto arbitrage_data_ringbuffer = Disruptor::RingBuffer<ArbitrageDataEvent>::createSingleProducer(
				[]() { return ArbitrageDataEvent(); },
				16,
				std::make_shared<Disruptor::BusySpinWaitStrategy>());

		std::shared_ptr<Messenger> messenger = std::make_shared<Messenger>(recorder);
		messenger->start();

		std::shared_ptr<LMAX::MarketOffice> lmax_market_office;
		std::shared_ptr<LMAX::TradeOffice> lmax_trade_office;
		std::shared_ptr<SWISSQUOTE::MarketOffice> swissquote_market_office;
		std::shared_ptr<SWISSQUOTE::TradeOffice> swissquote_trade_office;

		switch (broker) {
			case 1: {
				lmax_market_office = std::make_shared<LMAX::MarketOffice>(recorder,
				                                                          messenger,
				                                                          broker_market_data_disruptor,
				                                                          arbitrage_data_ringbuffer,
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
				                                                          lot_size);
				lmax_trade_office = std::make_shared<LMAX::TradeOffice>(recorder,
				                                                        messenger,
				                                                        arbitrage_data_ringbuffer,
				                                                        to_host,
				                                                        port,
				                                                        to_username,
				                                                        to_password,
				                                                        to_sender,
				                                                        to_receiver,
				                                                        heartbeat,
				                                                        diff_open,
				                                                        diff_close,
				                                                        lot_size);
			}
				lmax_trade_office->start();
				lmax_market_office->start();
				break;

			case 2: {
				swissquote_market_office = std::make_shared<SWISSQUOTE::MarketOffice>(recorder,
				                                                                      messenger,
				                                                                      broker_market_data_disruptor,
				                                                                      arbitrage_data_ringbuffer,
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
				                                                                      lot_size);
				swissquote_trade_office = std::make_shared<SWISSQUOTE::TradeOffice>(recorder,
				                                                                    messenger,
				                                                                    arbitrage_data_ringbuffer,
				                                                                    to_host,
				                                                                    port,
				                                                                    to_username,
				                                                                    to_password,
				                                                                    to_sender,
				                                                                    to_receiver,
				                                                                    heartbeat,
				                                                                    diff_open,
				                                                                    diff_close,
				                                                                    lot_size);
			}
				swissquote_trade_office->start();
				swissquote_market_office->start();
				break;

			default:
				recorder->recordSystem("Main: broker case FAILED", SYSTEM_RECORD_TYPE_ERROR);
				return EXIT_FAILURE;
		}

		task_scheduler->start(1);
		broker_market_data_disruptor->start();

		recorder->recordSystem("Main: all OK", SYSTEM_RECORD_TYPE_SUCCESS);

		while (true) {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}