
#ifndef ARBITO_MESSENGER_H
#define ARBITO_MESSENGER_H

// Aeron client
#include <Context.h>
#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <FragmentAssembler.h>

// Aeron media driver
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>

// Domain
#include "Recorder.h"
#include "MessengerConfig.h"

class Messenger {

public:
	explicit Messenger(const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
	                   Recorder &recorder, MessengerConfig config);

	const std::shared_ptr<aeron::Subscription> &marketDataSub() const;

	const std::shared_ptr<aeron::ExclusivePublication> &marketDataExPub() const;

	void start();

private:
	void mediaDriver();

private:
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	Recorder *m_recorder;
	MessengerConfig m_config;
	std::thread m_media_driver;
	aeron::Context m_aeron_context;
	std::shared_ptr<aeron::Aeron> m_aeron_client;
	std::shared_ptr<aeron::ExclusivePublication> m_market_data_ex_pub;
	std::shared_ptr<aeron::Subscription> m_market_data_sub;
};


#endif //ARBITO_MESSENGER_H
