
// Domain
#include "lmax/MarketOffice.h"
#include "lmax/TradeOffice.h"
#include "swissquote/MarketOffice.h"
#include "swissquote/TradeOffice.h"
#include "BusinessOffice.h"
#include "Messenger.h"
#include "Recorder.h"

// Disruptor
#include "Disruptor/Disruptor.h"
#include "Disruptor/BusySpinWaitStrategy.h"

using namespace Disruptor;
using namespace std;
using namespace std::chrono;

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

		int max_orders = atoi(getenv("MAX_ORDERS"));
		int md_delay = atoi(getenv("MD_DELAY"));

		const char *main_account = getenv("ACCOUNT"); // swissquote only

		const char *uri_string = getenv("MONGO_URI");
		const char *db_name = getenv("MONGO_DB");

		MessengerConfig messenger_config = (MessengerConfig) {
				.pub_channel = getenv("PUB_CHANNEL"),
				.sub_channel = getenv("SUB_CHANNEL"),
				.md_stream_id = atoi(getenv("MD_STREAM_ID"))
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

		auto remote_buffer = RingBuffer<RemoteMarketDataEvent>::createSingleProducer(
				[] { return RemoteMarketDataEvent(); },
				32,
				make_shared<BusySpinWaitStrategy>()
		);

		auto local_buffer = RingBuffer<MarketDataEvent>::createSingleProducer(
				[] { return MarketDataEvent(); },
				32,
				make_shared<BusySpinWaitStrategy>()
		);

		auto business_buffer = RingBuffer<BusinessEvent>::createSingleProducer(
				[] { return BusinessEvent(); },
				16,
				make_shared<BusySpinWaitStrategy>()
		);

		auto trade_buffer = RingBuffer<TradeEvent>::createSingleProducer(
				[] { return TradeEvent(); },
				16,
				make_shared<BusySpinWaitStrategy>()
		);

		auto control_buffer = RingBuffer<ControlEvent>::createMultiProducer(
				[] { return ControlEvent(); },
				16,
				make_shared<BusySpinWaitStrategy>()
		);

		pthread_setname_np(pthread_self(), "main");

		srand(static_cast<unsigned int>(time(nullptr)));

		Recorder recorder(
				local_buffer,
				remote_buffer,
				business_buffer,
				trade_buffer,
				control_buffer,
				uri_string,
				broker,
				db_name
		);
		recorder.start();

		Messenger messenger(
				control_buffer,
				local_buffer,
				remote_buffer,
				recorder,
				messenger_config
		);
		messenger.start();

		BusinessOffice bo(
				control_buffer,
				local_buffer,
				remote_buffer,
				business_buffer,
				recorder,
				diff_open,
				diff_close,
				lot_size,
				max_orders,
				md_delay
		);
		bo.start();

		LMAX::TradeOffice *lmax_to;
		LMAX::MarketOffice *lmax_mo;

		SWISSQUOTE::TradeOffice *swissquote_to;
		SWISSQUOTE::MarketOffice *swissquote_mo;

		switch (broker) {
			case 1:
				lmax_mo = new LMAX::MarketOffice(
						control_buffer,
						local_buffer,
						recorder,
						mo_config,
						spread,
						lot_size
				);
				lmax_to = new LMAX::TradeOffice(
						control_buffer,
						business_buffer,
						trade_buffer,
						recorder,
						to_config,
						lot_size
				);
				lmax_to->start();
				lmax_mo->start();
				break;

			case 2:
				swissquote_mo = new SWISSQUOTE::MarketOffice(
						control_buffer,
						local_buffer,
						recorder,
						mo_config,
						spread,
						lot_size
				);
				swissquote_to = new SWISSQUOTE::TradeOffice(
						control_buffer,
						business_buffer,
						trade_buffer,
						recorder,
						to_config,
						lot_size,
						main_account
				);
				swissquote_to->start();
				swissquote_mo->start();
				break;
			default:
				recorder.systemEvent("Main: Broker undefined", SE_TYPE_ERROR);
				return EXIT_FAILURE;
		}

		auto lower_bound = hours(20) + minutes(55);
		auto upper_bound = hours(21) + minutes(5);

		while (true) {
			this_thread::sleep_for(minutes(1));
			auto time_point = system_clock::now();
			time_t t = system_clock::to_time_t(time_point);
			auto gmt_time = gmtime(&t);
			auto now = hours(gmt_time->tm_hour) + minutes(gmt_time->tm_min);
			if (now >= lower_bound && now <= upper_bound) {
				recorder.systemEvent("END OF DAY", SE_TYPE_SUCCESS);
				return EXIT_SUCCESS;
			}
		}

	} catch (const exception &exception) {
		fprintf(stderr, "EXCEPTION: %s\n", exception.what());
	}

	return EXIT_FAILURE;
}