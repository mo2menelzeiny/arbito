
#include "catch2/catch.hpp"

// Disruptor
#include <Disruptor/BusySpinWaitStrategy.h>

// Domain
#include "StreamOffice.h"

TEST_CASE("Test StreamOffice construction", "[StreamOffice][unit]") {

	auto priceRingBuffer = Disruptor::RingBuffer<PriceEvent>::createMultiProducer(
			[]() { return PriceEvent(); },
			32,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	SECTION("should initialize new instance") {
		SECTION("without username and password") {
			auto *streamOffice = new StreamOffice(priceRingBuffer);
			REQUIRE(streamOffice != nullptr);
		}

		SECTION("With args") {
			auto *streamOffice = new StreamOffice(priceRingBuffer, "host", "username", "password");
			REQUIRE(streamOffice != nullptr);
		}
	}
}

TEST_CASE("Test StreamOffice run", "[StreamOffice][unit]") {
	auto priceRingBuffer = Disruptor::RingBuffer<PriceEvent>::createMultiProducer(
			[]() { return PriceEvent(); },
			32,
			std::make_shared<Disruptor::BusySpinWaitStrategy>()
	);

	StreamOffice streamOffice(priceRingBuffer);

	SECTION("should initiate") {
		streamOffice.initiate();
	}
}
