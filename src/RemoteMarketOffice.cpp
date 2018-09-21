
#include "RemoteMarketOffice.h"

RemoteMarketOffice::RemoteMarketOffice(
		const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer, Messenger &messenger)
		: m_control_buffer(control_buffer), m_remote_md_buffer(remote_md_buffer), m_messenger(&messenger) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void RemoteMarketOffice::start() {
	m_poller = std::thread(&RemoteMarketOffice::poll, this);
	m_poller.detach();
}

void RemoteMarketOffice::poll() {
	bool pause = false;
	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE:
				pause = true;
				break;

			case CET_RESUME:
				pause = false;
				break;

			default:
				break;
		}

		return false;
	};

	auto now_us = 0L;
	sbe::MessageHeader sbe_header;
	sbe::MarketData sbe_market_data;
	aeron::BusySpinIdleStrategy busySpinIdleStrategy;
	aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
	                                               aeron::index_t length, const aeron::Header &header) {
		if (pause) {
			return;
		}

		now_us = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();

		sbe_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSENGER_BUFFER_SIZE);
		sbe_market_data.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
		                              sbe::MessageHeader::encodedLength(), sbe_header.blockLength(),
		                              sbe_header.version(), MESSENGER_BUFFER_SIZE);

		auto next = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next] = (RemoteMarketDataEvent) {
				.bid = sbe_market_data.bid(),
				.offer = sbe_market_data.offer(),
				.timestamp_us  = sbe_market_data.timestamp(),
				.rec_timestamp_us = now_us
		};
		m_remote_md_buffer->publish(next);
	});

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		control_poller->poll(control_handler);
		busySpinIdleStrategy.idle(m_messenger->marketDataSub()->poll(fragmentAssembler.handler(), 1));
	}
}
