
#include "catch2/catch.hpp"
#include "nats/nats.h"

TEST_CASE("Connect to server", "[NATS Client]") {
	natsConnection *nc = nullptr;

	natsStatus s = natsConnection_ConnectTo(&nc, NATS_DEFAULT_URL);
	if (s == NATS_OK) {
		SUCCEED();
	} else {
		FAIL(s);
	}

}
