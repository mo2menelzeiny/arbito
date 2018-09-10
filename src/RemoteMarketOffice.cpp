
#include "RemoteMarketOffice.h"

RemoteMarketOffice::RemoteMarketOffice(
		const std::shared_ptr<Disruptor::RingBuffer<RemoteMarketDataEvent>> &remote_md_buffer, Messenger &messenger)
		: m_remote_md_buffer(remote_md_buffer), m_messenger(&messenger) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void RemoteMarketOffice::start() {
	m_poller = std::thread(&RemoteMarketOffice::poll, this);
	m_poller.detach();
}

void RemoteMarketOffice::poll() {
	auto now_us = 0L;
	sbe::MessageHeader sbe_header;
	sbe::MarketData sbe_market_data;
	aeron::BusySpinIdleStrategy busySpinIdleStrategy;
	aeron::FragmentAssembler fragmentAssembler([&](aeron::AtomicBuffer &buffer, aeron::index_t offset,
	                                               aeron::index_t length, const aeron::Header &header) {
		sbe_header.wrap(reinterpret_cast<char *>(buffer.buffer() + offset), 0, 0, MESSENGER_BUFFER_SIZE);
		sbe_market_data.wrapForDecode(reinterpret_cast<char *>(buffer.buffer() + offset),
		                              sbe::MessageHeader::encodedLength(), sbe_header.blockLength(),
		                              sbe_header.version(), MESSENGER_BUFFER_SIZE);
		auto next_sequence = m_remote_md_buffer->next();
		(*m_remote_md_buffer)[next_sequence] = (RemoteMarketDataEvent) {
				.bid = sbe_market_data.bid(),
				.bid_qty = sbe_market_data.bidQty(),
				.offer = sbe_market_data.offer(),
				.offer_qty = sbe_market_data.offerQty(),
				.timestamp_us  = sbe_market_data.timestamp(),
				.rec_timestamp_us = now_us
		};
		m_remote_md_buffer->publish(next_sequence);
	});

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		now_us = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
		busySpinIdleStrategy.idle(m_messenger->marketDataSub()->poll(fragmentAssembler.handler(), 1));
	}
}
