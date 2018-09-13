
#include "BusinessOffice.h"

BusinessOffice::BusinessOffice(const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                               const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                               const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
                               Recorder &recorder, double diff_open, double diff_close, double lot_size)
		: m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer),
		  m_business_buffer(business_buffer), m_recorder(&recorder), m_diff_open(diff_open),
		  m_diff_close(diff_close), m_lot_size(lot_size) {}

void BusinessOffice::start() {
	m_poller = std::thread(&BusinessOffice::poll, this);
	m_poller.detach();
}

void BusinessOffice::poll() {
	RemoteMarketDataEvent remote_md{.bid = -99.0, .offer = 99.0};
	std::deque<MarketDataEvent> local_md;
	bool order_delay_check = false;
	time_t order_delay = ORDER_DELAY_SEC;
	time_t order_delay_start = time(nullptr);
	long now_us = 0;

	auto trigger_handler = [&]() {
		if (order_delay_check && ((time(nullptr) - order_delay_start) < order_delay)) {
			return;
		}

		order_delay_check = false;

		if (!m_orders_count) {
			m_open_side = NONE;
		}

		for (int i = 0; i < local_md.size(); i++) {
			switch (m_open_side) {
				case OPEN_SELL:
					if (m_orders_count < MAX_OPEN_ORDERS &&
					    (remote_md.bid - local_md[i].offer) >= m_diff_open) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						++m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					if ((local_md[i].bid - remote_md.offer) >= m_diff_close) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						--m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					break;

				case OPEN_BUY:
					if (m_orders_count < MAX_OPEN_ORDERS &&
					    (local_md[i].bid - remote_md.offer) >= m_diff_open) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						++m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					if (remote_md.bid - local_md[i].offer >= m_diff_close) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						--m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					break;

				case NONE:
					if ((local_md[i].bid - remote_md.offer) >= m_diff_open) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						m_open_side = OPEN_BUY;
						++m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					if ((remote_md.bid - local_md[i].offer) >= m_diff_open) {
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us
						};
						m_business_buffer->publish(next);
						m_open_side = OPEN_SELL;
						++m_orders_count;
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						order_delay_check = true;
						return;
					}

					break;
			}
		}
	};

	auto local_md_poller = m_local_md_buffer->newPoller();
	m_local_md_buffer->addGatingSequences({local_md_poller->sequence()});
	auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		local_md.push_front(data);
		trigger_handler();
		return true;
	};

	auto remote_md_poller = m_remote_md_buffer->newPoller();
	m_remote_md_buffer->addGatingSequences({remote_md_poller->sequence()});
	auto remote_md_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		remote_md = data;
		trigger_handler();
		return true;
	};

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		now_us = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();

		if (local_md.size() > 1 && (now_us - local_md.back().timestamp_us > MD_DELAY_US)) {
			local_md.pop_back();
		}

		local_md_poller->poll(local_md_handler);
		remote_md_poller->poll(remote_md_handler);
	}
}