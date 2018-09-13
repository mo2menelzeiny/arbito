
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"
#include "BusinessOffice.h"
#include "RemoteMarketOffice.h"
#include "Messenger.h"
#include "Recorder.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/BusySpinWaitStrategy.h"

int main() {
	try {
		int broker = atoi(getenv("BROKER"));
		int port = atoi(getenv("PORT"));
		int heartbeat = atoi(getenv("HEARTBEAT"));

		double spread = atof(getenv("SPREAD"));
		double lot_size = atof(getenv("LOT_SIZE"));
		double diff_open = atof(getenv("DIFF_OPEN"));
		double diff_close = atof(getenv("DIFF_CLOSE"));

		const char *uri_string = getenv("MONGO_URI");
		const char *db_name = getenv("MONGO_DB");

		MessengerConfig messenger_config = (MessengerConfig) {
				.pub_channel = getenv("PUB_CHANNEL"),
				.sub_channel = getenv("SUB_CHANNEL"),
				.market_data_stream_id = atoi(getenv("MD_STREAM_ID"))
		};

		BrokerConfig mo_config = (BrokerConfig) {
				.host =  getenv("MO_HOST"),
				.username = getenv("MO_USERNAME"),
				.password = getenv("MO_PASSWORD"),
				.sender = getenv("MO_SENDER"),
				.receiver = getenv("MO_RECEIVER"),
				.port = port,
				.heartbeat = heartbeat
		};

		BrokerConfig to_config = (BrokerConfig) {
				.host =  getenv("TO_HOST"),
				.username = getenv("TO_USERNAME"),
				.password = getenv("TO_PASSWORD"),
				.sender = getenv("TO_SENDER"),
				.receiver = getenv("TO_RECEIVER"),
				.port = port,
				.heartbeat = heartbeat
		};

		auto remote_market_buffer = Disruptor::RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
				[]() { return RemoteMarketDataEvent(); }, 512, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto local_market_buffer = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
				[]() { return MarketDataEvent(); }, 512, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto business_buffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
				[]() { return BusinessEvent(); }, 512, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto trade_buffer = Disruptor::RingBuffer<TradeEvent>::createSingleProducer(
				[]() { return TradeEvent(); }, 512, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		srand(static_cast<unsigned int>(time(nullptr)));

		Recorder recorder(local_market_buffer, remote_market_buffer, business_buffer, trade_buffer, uri_string, broker,
		                  db_name);

		Messenger messenger(recorder, messenger_config);
		messenger.start();

		RemoteMarketOffice rmo(remote_market_buffer, messenger);
		rmo.start();

		BusinessOffice bo(local_market_buffer, remote_market_buffer, business_buffer, recorder, diff_open, diff_close,
		                  lot_size);
		bo.start();

		LMAX::TradeOffice *lmax_to;
		LMAX::MarketOffice *lmax_mo;

		SWISSQUOTE::TradeOffice *swissquote_to;
		SWISSQUOTE::MarketOffice *swissquote_mo;

		switch (broker) {
			case 1:
				lmax_mo = new LMAX::MarketOffice(local_market_buffer, recorder, messenger, mo_config, spread, lot_size);
				lmax_to = new LMAX::TradeOffice(business_buffer, trade_buffer, recorder, messenger, to_config,
				                                lot_size);
				lmax_to->start();
				lmax_mo->start();
				break;

			case 2:
				swissquote_mo = new SWISSQUOTE::MarketOffice(local_market_buffer, recorder, messenger, mo_config,
				                                             spread, lot_size);
				swissquote_to = new SWISSQUOTE::TradeOffice(business_buffer, trade_buffer, recorder, messenger,
				                                            to_config, lot_size);
				swissquote_to->start();
				swissquote_mo->start();
				break;
			default:
				return EXIT_FAILURE;
		}

		recorder.start();

		while (true) {
			std::this_thread::sleep_for(std::chrono::hours(1));
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << "\n";
	}

	return EXIT_FAILURE;
}