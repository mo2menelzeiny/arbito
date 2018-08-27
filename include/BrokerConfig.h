
#ifndef ARBITO_BROKERCONFIG_H
#define ARBITO_BROKERCONFIG_H


struct BrokerConfig {
	const char *host;
	const char *username;
	const char *password;
	const char *sender;
	const char *receiver;
	int port;
	int heartbeat;
};


#endif //ARBITO_BROKERCONFIG_H
