
#ifndef ARBITO_COMMUNICATIONOFFICE_H
#define ARBITO_COMMUNICATIONOFFICE_H

#define AERON_BUFFER_SIZE 1024

#include <thread>
#include <Aeron.h>
#include <concurrent/BusySpinIdleStrategy.h>
#include <FragmentAssembler.h>

#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>

#include <Disruptor/Disruptor.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ThreadPerTaskScheduler.h>
#include <Disruptor/BusySpinWaitStrategy.h>

#include "sbe/sbe.h"
#include "sbe/MarketData.h"
#include <cmath>
#include "Event.h"


struct AeronConfig {
	const char *pub_channel;
	int pub_stream_id;
	const char *sub_channel;
	int sub_stream_id;
};

class CommunicationOffice : public Disruptor::IEventHandler<Event> {

public:
	CommunicationOffice(const char *pub_channel, int pub_stream_id, const char *sub_channel, int sub_stream_id);

	template <class T> void addConsumer(const std::shared_ptr<T> consumer) {
		m_disruptor->handleEventsWith(consumer);
	};

	void start();

	void onEvent(Event &data, std::int64_t sequence, bool endOfBatch) override;

private:
	void mediaDriver();

	void initialize();

	void poll();

private:
	AeronConfig m_aeron_config;
	aeron::Context m_aeron_context;
	std::thread m_aeron_media_driver;
	std::thread m_polling_thread;
	std::shared_ptr<aeron::Aeron> m_aeron;
	std::shared_ptr<aeron::Publication> m_publication;
	std::shared_ptr<aeron::Subscription> m_subscription;
	std::shared_ptr<Disruptor::disruptor<Event>> m_disruptor;
	std::shared_ptr<Disruptor::ThreadPerTaskScheduler> m_taskscheduler;
	uint8_t m_buffer[AERON_BUFFER_SIZE];
};


#endif //ARBITO_COMMUNICATIONOFFICE_H
