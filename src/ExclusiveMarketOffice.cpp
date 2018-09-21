
#include "ExclusiveMarketOffice.h"

ExclusiveMarketOffice::ExclusiveMarketOffice(
		const std::shared_ptr<Disruptor::RingBuffer<ControlEvent>> &control_buffer,
		const std::shared_ptr<Disruptor::RingBuffer<MarketDataEvent>> &local_md_buffer, Messenger &messenger)
		: m_control_buffer(control_buffer), m_local_md_buffer(local_md_buffer), m_messenger(&messenger) {
	m_atomic_buffer.wrap(m_buffer, MESSENGER_BUFFER_SIZE);
}

void ExclusiveMarketOffice::start() {
	m_poller = std::thread(&ExclusiveMarketOffice::poll, this);
	m_poller.detach();
}

void ExclusiveMarketOffice::poll() {
	bool is_paused = false;
	sbe::MessageHeader sbe_header;
	sbe::MarketData sbe_market_data;

	auto control_poller = m_control_buffer->newPoller();
	m_control_buffer->addGatingSequences({control_poller->sequence()});
	auto control_handler = [&](ControlEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		switch (data.type) {
			case CET_PAUSE: {
				is_paused = true;
				auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count();
				sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
						.blockLength(sbe::MarketData::sbeBlockLength())
						.templateId(sbe::MarketData::sbeTemplateId())
						.schemaId(sbe::MarketData::sbeSchemaId())
						.version(sbe::MarketData::sbeSchemaVersion());
				sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
				                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
						.bid(-99)
						.offer(99)
						.timestamp(now_us);
				aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();

				while (m_messenger->marketDataExPub()->offer(m_atomic_buffer, 0, len) < -1);
			}
				break;

			case CET_RESUME:
				is_paused = false;
				break;

			default:
				break;
		}

		return false;
	};

	auto local_md_poller = m_local_md_buffer->newPoller();
	m_control_buffer->addGatingSequences({local_md_poller->sequence()});
	auto local_md_handler = [&](MarketDataEvent &data, std::int64_t sequence, bool endOfBatch) -> bool {
		if (is_paused) {
			return false;
		}

		sbe_header.wrap(reinterpret_cast<char *>(m_buffer), 0, 0, MESSENGER_BUFFER_SIZE)
				.blockLength(sbe::MarketData::sbeBlockLength())
				.templateId(sbe::MarketData::sbeTemplateId())
				.schemaId(sbe::MarketData::sbeSchemaId())
				.version(sbe::MarketData::sbeSchemaVersion());
		sbe_market_data.wrapForEncode(reinterpret_cast<char *>(m_buffer),
		                              sbe::MessageHeader::encodedLength(), MESSENGER_BUFFER_SIZE)
				.bid(data.bid)
				.offer(data.offer)
				.timestamp(data.timestamp_us);

		aeron::index_t len = sbe::MessageHeader::encodedLength() + sbe_market_data.encodedLength();

		while (m_messenger->marketDataExPub()->offer(m_atomic_buffer, 0, len) < -1);

		return false;
	};

	while (true) {
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		control_poller->poll(control_handler);
		local_md_poller->poll(local_md_handler);
	}
}
