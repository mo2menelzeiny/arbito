
#ifndef ARBITO_MEDIADRIVER_H
#define ARBITO_MEDIADRIVER_H


// Aeron media driver
#include <aeronmd/aeronmd.h>
#include <aeronmd/concurrent/aeron_atomic64_gcc_x86_64.h>
#include <aeron_driver_context.h>

// General
#include <cstring>
#include <cstdio>
#include <thread>

class MediaDriver {
public:
	MediaDriver();

private:
	void work();
};


#endif //ARBITO_MEDIADRIVER_H
