
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"
#include "BusinessOffice.h"
#include "RemoteMarketOffice.h"
#include "ExclusiveMarketOffice.h"
#include "Messenger.h"
#include "Recorder.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/BusySpinWaitStrategy.h"

int main() {
	try {
		int broker = atoi(getenv("BROKER"));
		int mo_port = atoi(getenv("MO_PORT"));
		int to_port = atoi(getenv("TO_PORT"));
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
				.port = mo_port,
				.heartbeat = heartbeat
		};

		BrokerConfig to_config = (BrokerConfig) {
				.host =  getenv("TO_HOST"),
				.username = getenv("TO_USERNAME"),
				.password = getenv("TO_PASSWORD"),
				.sender = getenv("TO_SENDER"),
				.receiver = getenv("TO_RECEIVER"),
				.port = to_port,
				.heartbeat = heartbeat
		};

		auto remote_buffer = Disruptor::RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
				[]() { return RemoteMarketDataEvent(); }, 16, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto local_buffer = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
				[]() { return MarketDataEvent(); }, 16, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto business_buffer = Disruptor::RingBuffer<BusinessEvent>::createSingleProducer(
				[]() { return BusinessEvent(); }, 8, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto trade_buffer = Disruptor::RingBuffer<TradeEvent>::createSingleProducer(
				[]() { return TradeEvent(); }, 8, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto control_buffer = Disruptor::RingBuffer<ControlEvent>::createMultiProducer(
				[]() { return ControlEvent(); }, 8, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		srand(static_cast<unsigned int>(time(nullptr)));

		Recorder recorder(local_buffer, remote_buffer, business_buffer, trade_buffer, control_buffer, uri_string,
		                  broker, db_name);

		Messenger messenger(remote_buffer, recorder, messenger_config);
		messenger.start();

		RemoteMarketOffice rmo(control_buffer, remote_buffer, messenger);
		rmo.start();

		ExclusiveMarketOffice emo(control_buffer, local_buffer, messenger);
		emo.start();

		BusinessOffice bo(control_buffer, local_buffer, remote_buffer, business_buffer, recorder, diff_open, diff_close,
		                  lot_size);
		bo.start();

		LMAX::TradeOffice *lmax_to;
		LMAX::MarketOffice *lmax_mo;

		SWISSQUOTE::TradeOffice *swissquote_to;
		SWISSQUOTE::MarketOffice *swissquote_mo;

		switch (broker) {
			case 1:
				lmax_mo = new LMAX::MarketOffice(control_buffer, local_buffer, recorder, mo_config, spread, lot_size);
				lmax_to = new LMAX::TradeOffice(control_buffer, business_buffer, trade_buffer, recorder, to_config,
				                                lot_size);
				lmax_mo->start();
				lmax_to->start();
				break;

			case 2:
				swissquote_mo = new SWISSQUOTE::MarketOffice(control_buffer, local_buffer, recorder, mo_config, spread,
				                                             lot_size);
				swissquote_to = new SWISSQUOTE::TradeOffice(control_buffer, business_buffer, trade_buffer, recorder,
				                                            to_config, lot_size);
				swissquote_mo->start();
				swissquote_to->start();
				break;
			default:
				recorder.recordSystemMessage("Main: Broker undefined", SYSTEM_RECORD_TYPE_ERROR);
				fprintf(stderr, "Main: Broker undefined\n");
				return EXIT_FAILURE;
		}

		recorder.start();

		auto lower_bound = std::chrono::hours(20) + std::chrono::minutes(55);
		auto upper_bound = std::chrono::hours(21) + std::chrono::minutes(5);

		while (true) {
			std::this_thread::sleep_for(std::chrono::minutes(1));
			auto time_point = std::chrono::system_clock::now();
			std::time_t t = std::chrono::system_clock::to_time_t(time_point);
			auto gmt_time = std::gmtime(&t);
			auto now = std::chrono::hours(gmt_time->tm_hour) + std::chrono::minutes(gmt_time->tm_min);
			if (now >= lower_bound && now <= upper_bound) {
				recorder.recordSystemMessage("END OF DAY", SYSTEM_RECORD_TYPE_SUCCESS);
				return EXIT_SUCCESS;
			}
		}

	} catch (const std::exception &exception) {
		fprintf(stderr, "EXCEPTION: %s\n", exception.what());
	}

	return EXIT_FAILURE;
}