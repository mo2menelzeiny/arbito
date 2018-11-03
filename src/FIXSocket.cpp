
#include "FIXSocket.h"

void FIXSocket::initiate() {
	this->configureSSL();

	struct hostent *host_ent = gethostbyname(m_host);

	if (!host_ent) {
		throw std::runtime_error("Host lookup FAILED");
	}

	char **ap;
	int saved_errno = 0;
	for (ap = host_ent->h_addr_list; *ap; ap++) {
		m_socketFD = socket(host_ent->h_addrtype, SOCK_STREAM, IPPROTO_TCP);

		if (m_socketFD < 0) {
			saved_errno = errno;
			continue;
		}

		struct sockaddr_in socket_address = (struct sockaddr_in) {
				.sin_family        = static_cast<sa_family_t>(host_ent->h_addrtype),
				.sin_port        = htons(static_cast<uint16_t>(m_port)),
		};

		memcpy(&socket_address.sin_addr, *ap, static_cast<size_t>(host_ent->h_length));

		if (connect(m_socketFD, (const struct sockaddr *) &socket_address, sizeof(struct sockaddr_in)) < 0) {
			saved_errno = errno;
			this->terminate();
			m_socketFD = -1;
			continue;
		}

		break;
	}

	if (m_socketFD < 0) {
		throw std::runtime_error("Socket connection FAILED" + std::to_string(saved_errno));
	}

	int optval = 1;
	if (setsockopt(m_socketFD, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
		throw std::runtime_error("Socket options FAILED");
	}

	SSL_set_fd(m_ssl, m_socketFD);
	int ssl_errno = SSL_connect(m_ssl);
	if (ssl_errno <= 0) {
		throw std::runtime_error("SSL FAILED");
	}

	fcntl(m_socketFD, F_SETFL, O_NONBLOCK);
}

void FIXSocket::terminate() {
	close(m_socketFD);
	SSL_free(m_ssl);
	ERR_free_strings();
	EVP_cleanup();
}

void FIXSocket::configureSSL() {
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	m_ssl_ctx = SSL_CTX_new(TLSv1_client_method());

	SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv3);
	SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv2);
	SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);

	SSL_CTX_use_certificate_file(m_ssl_ctx, "./resources/cert.pem", SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "./resources/key.pem", SSL_FILETYPE_PEM);

	m_ssl = SSL_new(m_ssl_ctx);
}
