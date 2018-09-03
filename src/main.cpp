
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/RingBuffer.h"
#include "Disruptor/RoundRobinThreadAffinedTaskScheduler.h"
#include "Disruptor/BusySpinWaitStrategy.h"

// MongoDB
#include <mongoc.h>

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

		MessengerConfig mo_messenger_config = (MessengerConfig) {
				.pub_channel = getenv("PUB_CHANNEL"),
				.sub_channel = getenv("SUB_CHANNEL"),
				.stream_id = atoi(getenv("MO_STREAM_ID"))
		};

		BrokerConfig mo_broker_config = (BrokerConfig) {
				.host =  getenv("MO_HOST"),
				.username = getenv("MO_USERNAME"),
				.password = getenv("MO_PASSWORD"),
				.sender = getenv("MO_SENDER"),
				.receiver = getenv("MO_RECEIVER"),
				.port = port,
				.heartbeat = heartbeat
		};

		MessengerConfig to_messenger_config = (MessengerConfig) {
				.pub_channel = getenv("PUB_CHANNEL"),
				.sub_channel = getenv("SUB_CHANNEL"),
				.stream_id = atoi(getenv("TO_STREAM_ID"))
		};

		BrokerConfig to_broker_config = (BrokerConfig) {
				.host =  getenv("TO_HOST"),
				.username = getenv("TO_USERNAME"),
				.password = getenv("TO_PASSWORD"),
				.sender = getenv("TO_SENDER"),
				.receiver = getenv("TO_RECEIVER"),
				.port = port,
				.heartbeat = heartbeat
		};

		fprintf(stdout, "Main: System starting..\n");

		Recorder recorder(uri_string, broker, db_name);

		recorder.recordSystem("Main: initialize OK", SYSTEM_RECORD_TYPE_SUCCESS);

		auto local_md_ringbuff = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
				[]() { return MarketDataEvent(); }, 8, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		auto remote_md_ringbuff = Disruptor::RingBuffer<MarketDataEvent>::createSingleProducer(
				[]() { return MarketDataEvent(); }, 8, std::make_shared<Disruptor::BusySpinWaitStrategy>());

		Messenger messenger(recorder);
		messenger.start();

		LMAX::MarketOffice *lmax_mo;
		LMAX::TradeOffice *lmax_to;

		SWISSQUOTE::MarketOffice *swissquote_mo;
		SWISSQUOTE::TradeOffice *swissquote_to;

		switch (broker) {
			case 1:
				lmax_mo = new LMAX::MarketOffice(recorder, messenger, local_md_ringbuff, remote_md_ringbuff,
				                                 mo_messenger_config, mo_broker_config, spread, lot_size);
				lmax_to = new LMAX::TradeOffice(recorder, messenger, local_md_ringbuff, remote_md_ringbuff,
				                                to_messenger_config, to_broker_config, diff_open, diff_close, lot_size);
				lmax_to->start();
				lmax_mo->start();
				break;

			case 2:
				swissquote_mo = new SWISSQUOTE::MarketOffice(recorder, messenger, local_md_ringbuff, remote_md_ringbuff,
				                                             mo_messenger_config, mo_broker_config, spread, lot_size);
				swissquote_to = new SWISSQUOTE::TradeOffice(recorder, messenger, local_md_ringbuff, remote_md_ringbuff,
				                                            to_messenger_config, to_broker_config, diff_open,
				                                            diff_close, lot_size);
				swissquote_to->start();
				swissquote_mo->start();
				break;
			default:
				recorder.recordSystem("Main: broker case FAILED", SYSTEM_RECORD_TYPE_ERROR);
				return EXIT_FAILURE;
		}

		recorder.recordSystem("Main: all OK", SYSTEM_RECORD_TYPE_SUCCESS);

		while (true) {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

	} catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
	}

	return EXIT_FAILURE;
}