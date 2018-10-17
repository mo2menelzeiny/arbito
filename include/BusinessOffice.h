
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
#include "ControlEvent.h"
#include "BusinessState.h"


class BusinessOffice {

public:
	BusinessOffice(
			const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
			const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
			Recorder &recorder,
			double diff_open,
			double diff_close,
			double lot_size,
			int max_orders,
			int local_delay
	);

	void start();

private:
	void poll();

private:
	const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> m_control_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> m_local_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> m_remote_md_buffer;
	const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> m_business_buffer;
	Recorder *m_recorder;
	double m_diff_open;
	double m_diff_close;
	double m_lot_size;
	int m_max_orders;
	int m_local_delay;
	BusinessState m_business_state;
};


#endif //ARBITO_BUSINESSOFFICE_H
