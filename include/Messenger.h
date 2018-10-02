
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

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain
#include "Config.h"
#include "Recorder.h"
#include "MessengerConfig.h"

class Messenger {

public:
	explicit Messenger(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
	                   const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
	                   const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
	                   Recorder &recorder, MessengerConfig config);

	void start();

private:
	void mediaDriver();

	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	Recorder *m_recorder;
	MessengerConfig m_config;
	std::thread m_poller;
	std::thread m_media_driver;
	aeron::Context m_aeron_context;
	std::shared_ptr<aeron::Aeron> m_aeron_client;
	std::shared_ptr<aeron::ExclusivePublication> m_market_data_ex_pub;
	std::shared_ptr<aeron::Subscription> m_market_data_sub;
	uint8_t m_buffer[MESSENGER_BUFFER_SIZE];
	aeron::concurrent::AtomicBuffer m_atomic_buffer;
};


#endif //ARBITO_MESSENGER_H
