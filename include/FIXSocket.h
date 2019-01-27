
#ifndef ARBITO_FIXSOCKET_H
#define ARBITO_FIXSOCKET_H

// Socket
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>

// OpenSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// General
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstring>

class FIXSocket {
public:
	FIXSocket(const char *host, int port, bool sslEnabled) : m_host(host), m_port(port), m_sslEnabled(sslEnabled) {}

	inline int socketFD() const {
		return m_socketFD;
	}

	inline SSL *ssl() const {
		return m_ssl;
	}

	void initiate();

	void terminate();

private:
	const char *m_host;
	int m_port;
	int m_socketFD{};
	bool m_sslEnabled;
	SSL_CTX *m_ssl_ctx{};
	SSL *m_ssl{};
};


#endif //ARBITO_FIXSOCKET_H
