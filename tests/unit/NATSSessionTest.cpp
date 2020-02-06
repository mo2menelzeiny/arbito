#include "catch2/catch.hpp"

// Domain
#include "NATSSession.h"

TEST_CASE("Test NATSSession construction", "[NATSSession][unit]") {

	SECTION("should initialize new instance") {

		SECTION("without arguments") {
			auto *natsSession = new NATSSession();
			REQUIRE(natsSession != nullptr);
			delete natsSession;
		}

		SECTION("with arguments") {
			auto *natsSession = new NATSSession("", "", "");
			REQUIRE(natsSession != nullptr);
			delete natsSession;
		}
	}
}


TEST_CASE("TEST NATSSession initiation", "[NATSSession][unit]") {

	auto create_fn = [](natsOptions **) {
		return NATS_OK;
	};

	auto setSendAsap_fn = [](natsOptions *, bool) {
		return NATS_OK;
	};

	auto setURL_fn = [](natsOptions *, const char *) {
		return NATS_OK;
	};

	auto connect_fn = [](natsConnection **, natsOptions *) {
		return NATS_OK;
	};

	auto destroy_fn = [](natsOptions *) {};

	SECTION("should set NATS options") {
		NATSSession natsSession;
		natsSession.initiate(
				create_fn,
				setSendAsap_fn,
				setURL_fn,
				connect_fn,
				natsOptions_Destroy
		);
	}

	SECTION("should throw on create opts nats error") {
		auto err_fn = [](natsOptions **) {
			return NATS_ERR;
		};

		NATSSession natsSession;
		REQUIRE_THROWS(
				natsSession.initiate(
						err_fn,
						setSendAsap_fn,
						setURL_fn,
						connect_fn,
						natsOptions_Destroy
				)
		);
	}

	SECTION("should throw on set asap nats error") {
		auto err_fn = [](natsOptions *, bool) {
			return NATS_ERR;
		};

		NATSSession natsSession;
		REQUIRE_THROWS(
				natsSession.initiate(
						create_fn,
						err_fn,
						setURL_fn,
						connect_fn,
						natsOptions_Destroy
				)
		);
	}

	SECTION("should throw on nats set url error") {
		auto err_fn = [](natsOptions *, const char *) {
			return NATS_ERR;
		};

		NATSSession natsSession;
		REQUIRE_THROWS(
				natsSession.initiate(
						create_fn,
						setSendAsap_fn,
						err_fn,
						connect_fn,
						natsOptions_Destroy
				)
		);
	}

	SECTION("should throw on nats connect error") {
		auto err_fn = [](natsConnection **, natsOptions *) {
			return NATS_ERR;
		};

		NATSSession natsSession;
		REQUIRE_THROWS(
				natsSession.initiate(
						create_fn,
						setSendAsap_fn,
						setURL_fn,
						err_fn,
						natsOptions_Destroy
				)
		);
	}
}

TEST_CASE("TEST NATSSession termination", "[NATSSession][unit]") {

	SECTION("should terminate session") {
		auto close_fn = [](natsConnection *) {};
		auto destroy_fn = [](natsConnection *) {};
		NATSSession natsSession;
		natsSession.terminate(
				close_fn,
				destroy_fn
		);
	}
}

TEST_CASE("TEST NATSSession publish string", "[NATSSession][unit]") {

	SECTION("should publish string") {
		auto pub_fn = [](natsConnection *, const char *, const char *) {
			return NATS_OK;
		};

		NATSSession natsSession;
		natsSession.publishString(
				pub_fn,
				"foo",
				"hello foo!"
		);
	}

	SECTION("should throw on failed publish") {
		auto err_fn = [](natsConnection *, const char *, const char *) {
			return NATS_ERR;
		};

		NATSSession natsSession;
		REQUIRE_THROWS(
				natsSession.publishString(
						err_fn,
						"foo",
						"hello foo!"
				)
		);
	}
}