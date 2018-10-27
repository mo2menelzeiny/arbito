
#ifndef ARBITO_MESSENGER_H
#define ARBITO_MESSENGER_H

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/NoOpIdleStrategy.h>
#include <FragmentAssembler.h>

// Aeron media driver
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>
#include <aeron_driver_context.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "Config.h"
#include "Recorder.h"
#include "MessengerConfig.h"

using namespace Disruptor;
using namespace aeron;
using namespace sbe;
using namespace std;
using namespace std::chrono;

class Messenger {

public:
	explicit Messenger(
			const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
			const shared_ptr<RingBuffer<MarketDataEvent>> &local_md_buffer,
			const shared_ptr<RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
			Recorder &recorder,
			MessengerConfig config
	);

	void start();

private:
	void mediaDriver();

	void poll();

private:
	const shared_ptr<RingBuffer<ControlEvent>> m_control_buffer;
	const shared_ptr<RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const shared_ptr<RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	Recorder *m_recorder;
	MessengerConfig m_config;
	Context m_aeron_context;
	shared_ptr<Aeron> m_aeron_client;
	shared_ptr<ExclusivePublication> m_market_data_ex_pub;
	shared_ptr<Subscription> m_market_data_sub;
	uint8_t m_buffer[MESSENGER_BUFFER_SIZE];
	concurrent::AtomicBuffer m_atomic_buffer;
};


#endif //ARBITO_MESSENGER_H
