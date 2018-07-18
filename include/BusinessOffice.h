
#ifndef ARBITO_BUSINESSOFFICE_H
#define ARBITO_BUSINESSOFFICE_H

#define MAX_DEALS 5

#include <Disruptor/Disruptor.h>
#include <Disruptor/IEventHandler.h>
#include <Disruptor/ThreadPerTaskScheduler.h>

#include "Event.h"
#include "OrderEvent.h"
struct Deal {
	unsigned long id;
};

class BusinessOffice : public Disruptor::IEventHandler<Event> {

public:
	BusinessOffice(double spread, double diff_open, double diff_close);

	template <class T> void addConsumer(const std::shared_ptr<T> consumer) {
		m_disruptor->handleEventsWith(consumer);
	};

	void onEvent(Event &data, std::int64_t sequence, bool endOfBatch) override;

private:
	double m_spread;
	double m_diff_open, m_diff_close;
	double m_bid_two, m_bid_two_size;
	double m_offer_two, m_offer_two_size;
	Deal m_deals[MAX_DEALS];
	std::shared_ptr<Disruptor::disruptor<OrderEvent>> m_disruptor;
	std::shared_ptr<Disruptor::ThreadPerTaskScheduler> m_taskscheduler;
};


#endif //ARBITO_BUSINESSOFFICE_H
