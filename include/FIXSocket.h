
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

class FIXSocket {
public:
	FIXSocket(const char *host, int port) : m_host(host), m_port(port) {}

	inline int socketFD() const {
		return m_socketFD;
	};

	void initiate();

	void terminate();

private:
	void configureSSL();

private:
	const char *m_host;
	int m_port;
	int m_socketFD{};
	SSL_CTX *m_ssl_ctx{};
	SSL *m_ssl{};
};


#endif //ARBITO_FIXSOCKET_H
