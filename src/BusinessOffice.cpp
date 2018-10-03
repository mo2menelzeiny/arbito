
#include "BusinessOffice.h"

using namespace std::chrono;

BusinessOffice::BusinessOffice(const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
                               const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer,
                               const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
                               const std::shared_ptr<Disruptor::RingBuffer<BusinessEvent>> &business_buffer,
                               Recorder &recorder, double diff_open, double diff_close, double lot_size)
		: m_control_buffer(control_buffer), m_local_md_buffer(local_md_buffer), m_remote_md_buffer(remote_md_buffer),
		  m_business_buffer(business_buffer), m_recorder(&recorder), m_diff_open(diff_open), m_diff_close(diff_close),
		  m_lot_size(lot_size) {
	m_business_state = (BusinessState) {
			.open_side = NONE,
			.orders_count = 0
	};
}

void BusinessOffice::start() {
	m_business_state = m_recorder->fetchBusinessState();
	m_poller = std::thread(&BusinessOffice::poll, this);
	m_poller.detach();
}

void BusinessOffice::poll() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "business");

	bool is_paused = false;
	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE:
				is_paused = true;
				break;
			case CET_RESUME:
				is_paused = false;
				break;
			default:
				break;
		}

		return false;
	};

	RemoteMarketDataEvent remote_md{.bid = -99.0, .offer = 99.0};
	std::deque<MarketDataEvent> local_md;
	bool is_order_delayed = false;
	time_t order_delay = ORDER_DELAY_SEC;
	time_t order_delay_start = time(nullptr);
	long now_us = 0;

	auto trigger_handler = [&]() {
		if (is_paused) {
			return;
		}

		if (is_order_delayed && ((time(nullptr) - order_delay_start) < order_delay)) {
			return;
		}

		is_order_delayed = false;

		if (!m_business_state.orders_count) {
			m_business_state.open_side = NONE;
		}

		for (std::size_t i = 0; i < local_md.size(); i++) {
			switch (m_business_state.open_side) {
				case OPEN_BUY:
					if (m_business_state.orders_count < MAX_OPEN_ORDERS &&
					    (remote_md.bid - local_md[i].offer) >= m_diff_open) {
						++m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if ((local_md[i].bid - remote_md.offer) >= m_diff_close) {
						--m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					break;

				case OPEN_SELL:
					if (m_business_state.orders_count < MAX_OPEN_ORDERS &&
					    (local_md[i].bid - remote_md.offer) >= m_diff_open) {
						++m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if (remote_md.bid - local_md[i].offer >= m_diff_close) {
						--m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					break;

				case NONE:
					if ((local_md[i].bid - remote_md.offer) >= m_diff_open) {
						m_business_state.open_side = OPEN_SELL;
						++m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_md[i].bid,
								.remote_px = remote_md.offer,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if ((remote_md.bid - local_md[i].offer) >= m_diff_open) {
						m_business_state.open_side = OPEN_BUY;
						++m_business_state.orders_count;
						auto next = m_business_buffer->next();
						(*m_business_buffer)[next] = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_md[i].offer,
								.remote_px = remote_md.bid,
								.timestamp_us = now_us,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};
						m_business_buffer->publish(next);
						local_md.erase(local_md.begin() + i);
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					break;
			}
		}
	};

	auto local_md_poller = m_local_md_buffer->newPoller();
	m_local_md_buffer->addGatingSequences({local_md_poller->sequence()});
	auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		if (local_md.size() == 1 && (now_us - local_md.front().timestamp_us > MD_DELAY_US)) {
			local_md.front().timestamp_us = now_us;
		}

		local_md.push_front(data);
		trigger_handler();
		return false;
	};

	auto remote_md_poller = m_remote_md_buffer->newPoller();
	m_remote_md_buffer->addGatingSequences({remote_md_poller->sequence()});
	auto remote_md_handler = [&](RemoteMarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		remote_md = data;
		trigger_handler();
		return false;
	};

	while (true) {
		now_us = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();

		if (local_md.size() > 1 && (now_us - local_md.back().timestamp_us > MD_DELAY_US)) {
			local_md.pop_back();
		}

		control_poller->poll(control_handler);
		local_md_poller->poll(local_md_handler);
		remote_md_poller->poll(remote_md_handler);
	}
}