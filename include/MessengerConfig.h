
#ifndef ARBITO_MESSENGERCONFIG_H
#define ARBITO_MESSENGERCONFIG_H

// struct to contain the Aeron configurations parameters
struct MessengerConfig {
	const char *pub_channel;
	const char *sub_channel;
	int stream_id;
};


#endif //ARBITO_MESSENGERCONFIG_H
