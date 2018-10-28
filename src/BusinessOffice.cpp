
#include "BusinessOffice.h"

BusinessOffice::BusinessOffice(
		const shared_ptr<RingBuffer<ControlEvent>> &control_buffer,
		const shared_ptr<RingBuffer<MarketDataEvent>> &local_md_buffer,
		const shared_ptr<RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer,
		const shared_ptr<RingBuffer<BusinessEvent>> &business_buffer,
		Recorder &recorder,
		double diff_open,
		double diff_close,
		double lot_size,
		int max_orders,
		int md_delay
) : m_control_buffer(control_buffer),
    m_local_md_buffer(local_md_buffer),
    m_remote_md_buffer(remote_md_buffer),
    m_business_buffer(business_buffer),
    m_recorder(&recorder),
    m_diff_open(diff_open),
    m_diff_close(diff_close),
    m_lot_size(lot_size),
    m_max_orders(max_orders),
    m_md_delay(md_delay) {
	m_business_state = (BusinessState) {
			.open_side = NONE,
			.orders_count = 0
	};
}

void BusinessOffice::start() {
	m_business_state = m_recorder->businessState();
	auto poller = thread(&BusinessOffice::poll, this);
	poller.detach();
	stringstream ss;
	ss << "BusinessOffice: Open side: "
	   << m_business_state.open_side
	   << " Orders count: "
	   << m_business_state.orders_count;
	const string &tmp = ss.str();
	const char *cstr = tmp.c_str();
	m_recorder->systemEvent(cstr, SE_TYPE_SUCCESS);
}

void BusinessOffice::poll() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(pthread_self(), "business");

	deque<MarketDataEvent> local_md;
	RemoteMarketDataEvent remote_md{.bid = -99.0, .offer = 99.0};

	bool is_paused = false;
	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE:
				is_paused = true;
				local_md.clear();
				remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
				break;
			case CET_RESUME:
				is_paused = false;
				break;
			default:
				break;
		}

		return false;
	};

	bool is_order_delayed = false;
	time_t order_delay = ORDER_DELAY_SEC;
	time_t order_delay_start = time(nullptr);
	long now_ms = 0;

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

		bool can_open = m_business_state.orders_count < m_max_orders;
		
		for (auto &local_idx : local_md) {
			double diff_1 = local_idx.bid - remote_md.offer;
			double diff_2 = remote_md.bid - local_idx.offer;

			switch (m_business_state.open_side) {
				case OPEN_BUY:
					if (can_open && diff_2 >= m_diff_open) {
						++m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_idx.offer,
								.remote_px = remote_md.bid,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if (diff_1 >= m_diff_close) {
						--m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_idx.bid,
								.remote_px = remote_md.offer,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					break;

				case OPEN_SELL:
					if (can_open && diff_1 >= m_diff_open) {
						++m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_idx.bid,
								.remote_px = remote_md.offer,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if (diff_2 >= m_diff_close) {
						--m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_idx.offer,
								.remote_px = remote_md.bid,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					break;

				case NONE:
					if (diff_1 >= m_diff_open) {
						m_business_state.open_side = OPEN_SELL;
						++m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '2',
								.clOrdId = rand(),
								.trigger_px = local_idx.bid,
								.remote_px = remote_md.offer,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
						order_delay_start = time(nullptr);
						is_order_delayed = true;
						return;
					}

					if (diff_2 >= m_diff_open) {
						m_business_state.open_side = OPEN_BUY;
						++m_business_state.orders_count;
						auto data = (BusinessEvent) {
								.side = '1',
								.clOrdId = rand(),
								.trigger_px = local_idx.offer,
								.remote_px = remote_md.bid,
								.timestamp_ms = now_ms,
								.open_side = m_business_state.open_side,
								.orders_count = m_business_state.orders_count
						};

						try {
							auto next = m_business_buffer->tryNext();
							(*m_business_buffer)[next] = data;
							m_business_buffer->publish(next);
						} catch (InsufficientCapacityException &e) {
							m_recorder->systemEvent(
									"BusinessOffice: Business buffer InsufficientCapacityException",
									SE_TYPE_ERROR
							);
						}

						local_md.clear();
						remote_md = (RemoteMarketDataEvent) {.bid = -99.0, .offer = 99.0};
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
	auto local_md_handler = [&](MarketDataEvent &data, int64_t sequence, bool endOfBatch) -> bool {
		if (local_md.size() == 1 && (now_ms - local_md.front().timestamp_ms > m_md_delay)) {
			local_md.front().timestamp_ms = now_ms;
		}
		local_md.push_front(data);
		return false;
	};

	auto remote_md_poller = m_remote_md_buffer->newPoller();
	m_remote_md_buffer->addGatingSequences({remote_md_poller->sequence()});
	auto remote_md_handler = [&](RemoteMarketDataEvent &data, int64_t sequence, bool endOfBatch) -> bool {
		remote_md = data;
		return false;
	};

	while (true) {
		now_ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();

		if (local_md.size() > 1 && (now_ms - local_md.back().timestamp_ms > m_md_delay)) {
			local_md.pop_back();
		}

		control_poller->poll(control_handler);
		local_md_poller->poll(local_md_handler);
		remote_md_poller->poll(remote_md_handler);

		trigger_handler();
	}
}