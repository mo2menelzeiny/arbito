
#ifndef ARBITO_BUSINESSOFFICE_H
#define ARBITO_BUSINESSOFFICE_H

// Disruptor includes
#include <Disruptor/Disruptor.h>

// Domain includes
#include "Config.h"
#include "Recorder.h"
#include "MarketDataEvent.h"
#include "RemoteMarketDataEvent.h"
#include "BusinessEvent.h"
#include "OpenSide.h"


class BusinessOffice {

public:
	BusinessOffice(
			const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer, Recorder &recorder,
			double diff_open, double diff_close, double lot_size);

	void start();

private:
	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_buffer;
	Recorder *m_recorder;
	double m_diff_open;
	double m_diff_close;
	double m_lot_size;
	OpenSide m_open_side = NONE;
	int m_orders_count = 0;
	std::thread m_poller;
};


#endif //ARBITO_BUSINESSOFFICE_H
