
#include "tests.h"
#include <MongoDBDriver.h>
#include <thread>

const char *uri = "mongodb+srv://beeks-nj:atlas_6852@arbito-htwdm.mongodb.net/?retryWrites=true";

const char *db = "arbito_local";

TEST_CASE("Initialize and validate URI", "[Mongodb driver]") {

	try {
		MongoDBDriver(uri, db, "orders");
	} catch (std::exception &ex) {
		FAIL(ex.what());
	}
}

TEST_CASE("Record trigger", "[Mongodb driver]") {

	try {
		MongoDBDriver mongoDBDriver(uri, db, "triggers");
		mongoDBDriver.record("TEST", 0, 0, "TEST");
	} catch (std::exception &ex) {
		FAIL(ex.what());
	}
}

TEST_CASE("Record order", "[Mongodb driver]") {

	try {
		MongoDBDriver mongoDBDriver(uri, db, "orders");
		mongoDBDriver.record("TEST", "TEST", 0, 0, "TEST", false);
	} catch (std::exception &ex) {
		FAIL(ex.what());
	}
}

TEST_CASE("Thread safe record", "[Mongodb driver]") {

	auto record = [=] {
		try {
			MongoDBDriver mongoDBDriver(uri, db, "orders");
			mongoDBDriver.record("TEST", "TEST", 0, 0, "TEST", false);
		} catch (std::exception &ex) {
			FAIL(ex.what());
		}
	};

	std::thread threadA(record);
	std::thread threadB(record);
	std::thread threadC(record);
	std::thread threadD(record);

	threadA.join();
	threadB.join();
	threadC.join();
	threadD.join();
}