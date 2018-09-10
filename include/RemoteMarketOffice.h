
#ifndef ARBITO_REMOTEMARKETOFFICE_H
#define ARBITO_REMOTEMARKETOFFICE_H

// Disruptor includes
#include <Disruptor/Disruptor.h>

// Aeron client
#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <FragmentAssembler.h>

// SBE
#include "sbe/sbe.h"
#include "sbe/MarketData.h"

// Domain includes
#include "Config.h"
#include "Messenger.h"
#include "RemoteMarketDataEvent.h"

class RemoteMarketOffice {

public:
	RemoteMarketOffice(const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
	                   Messenger &messenger);

	void start();

private:
	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	Messenger *m_messenger;
	std::thread m_poller;
	uint8_t m_buffer[MESSENGER_BUFFER_SIZE];
	aeron::concurrent::AtomicBuffer m_atomic_buffer;
};


#endif //ARBITO_REMOTEMARKETOFFICE_H
