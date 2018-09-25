
#include "RemoteMarketOffice.h"

RemoteMarketOffice::RemoteMarketOffice(
		const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer, Messenger &messenger)
		: m_control_buffer(control_buffer), m_remote_md_buffer(remote_md_buffer), m_messenger(&messenger) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void RemoteMarketOffice::start() {
	m_poller = std::thread(&RemoteMarketOffice::poll, this);
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	pthread_setaffinity_np(m_poller.native_handle(), sizeof(cpu_set_t), &cpuset);
	pthread_setname_np(m_poller.native_handle(), "remote");
	m_poller.detach();
}

void RemoteMarketOffice::poll() {
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

	sbe::MessageHeader sbe_header;
	sbe::MarketData sbe_market_data;
	aeron::NoOpIdleStrategy noOpIdleStrategy;
	aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
	                                               aeron::index_t length, const aeron::Header &header) {
		if (is_paused) {
			return;
		}

		sbe_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSENGER_BUFFER_SIZE);
		sbe_market_data.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
		                              sbe::MessageHeader::encodedLength(), sbe_header.blockLength(),
		                              sbe_header.version(), MESSENGER_BUFFER_SIZE);

		auto next = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next] = (RemoteMarketDataEvent) {
				.bid = sbe_market_data.bid(),
				.offer = sbe_market_data.offer(),
				.timestamp_us  = sbe_market_data.timestamp(),
				.rec_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count()
		};
		m_remote_md_buffer->publish(next);
	});

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		control_poller->poll(control_handler);
		noOpIdleStrategy.idle(m_messenger->marketDataSub()->poll(fragmentAssembler.handler(), 10));
	}
}
