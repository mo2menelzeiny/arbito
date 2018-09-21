
#ifndef ARBITO_EXCLUSIVEMARKETOFFICE_H
#define ARBITO_EXCLUSIVEMARKETOFFICE_H

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
#include "MarketDataEvent.h"
#include "ControlEvent.h"

class ExclusiveMarketOffice {

public:
	ExclusiveMarketOffice(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
	                   const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
	                   Messenger &messenger);

	void start();

private:
	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
	Messenger *m_messenger;
	std::thread m_poller;
	uint8_t m_buffer[MESSENGER_BUFFER_SIZE];
	aeron::concurrent::AtomicBuffer m_atomic_buffer;
};


#endif //ARBITO_EXCLUSIVEMARKETOFFICE_H
