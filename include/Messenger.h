
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

class Messenger {

public:
	Messenger(const std::shared_ptr<Recorder> &recorder);
	const std::shared_ptr<aeron::Aeron> &aeronClient() const;

private:
	void mediaDriver();
	void initialize();

private:
	aeron::Context m_aeron_context;
	std::shared_ptr<aeron::Aeron> m_aeron_client;
	std::thread m_media_driver;
	const std::shared_ptr<Recorder> m_recorder;
};


#endif //ARBITO_MESSENGER_H
