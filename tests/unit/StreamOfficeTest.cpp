
#include "catch2/catch.hpp"
#include "StreamOffice.h"

TEST_CASE("Test StreamOffice initialization", "[StreamOffice][unit]") {
	auto *streamOffice = new StreamOffice();

	SECTION("should initialize new instance") {
		REQUIRE_FALSE(streamOffice != nullptr);
	}

}
