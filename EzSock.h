#ifndef __EzSock_H__
#define __EzSock_H__

#define _WINSOCKAPI_
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <fcntl.h>
#include <cctype>

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

class EzSock
{
public:
	enum SockState
	{
		skDISCONNECTED = 0,
		skUNDEF1, //Not implemented
		skLISTENING,
		skUNDEF3, //Not implemented
		skUNDEF4, //Not implemented
		skUNDEF5, //Not implemented
		skUNDEF6, //Not implemented
		skCONNECTED,
		skERROR
	};

	bool blocking;
	bool valid;

	struct sockaddr_in addr;
	struct sockaddr_in fromAddr;
	unsigned long fromAddr_len;

	SockState state;

	int lastCode;

	EzSock();
	~EzSock();

	bool create();
	bool create(int Protocol);
	bool create(int Protocol, int Type);
	void setBlocking(bool blocking);
	bool bind(unsigned short port);
	bool listen();
	bool accept(EzSock* socket);
	int connect(const char* host, unsigned short port);
	void close();

	long uAddr();
	bool isError();

	bool canRead();
	bool canWrite();

	int sock;
	int receive(const void* buffer, int size, int spos = 0);
	int sendRaw(const void* data, int dataSize);
	int sendUDP(const void* buffer, int size, sockaddr_in* to);
	int receiveUDP(const void* buffer, int size, sockaddr_in* from);

private:
#ifdef _MSC_VER
	WSADATA wsda;
#endif
	int MAXCON;

	fd_set scks;
	timeval times;

	unsigned int totaldata;
	bool check();
};

#endif

#ifdef EZS_CPP
#ifndef __EzSock_CPP__
#define __EzSock_CPP__

#ifdef _MSC_VER
#pragma comment(lib,"wsock32.lib")
typedef int socklen_t;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#endif

#if !defined(SOCKET_ERROR)
#define SOCKET_ERROR -1
#endif

#if !defined(SOCKET_NONE)
#define SOCKET_NONE 0
#endif

#if !defined(INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif

EzSock::EzSock()
{
	MAXCON = 64;
	memset(&addr, 0, sizeof(addr));

#ifdef _MSC_VER
	WSAStartup(MAKEWORD(1, 1), &wsda);
#endif

	this->sock = INVALID_SOCKET;
	this->blocking = false;
	this->valid = false;
	this->times.tv_sec = 0;
	this->times.tv_usec = 0;
	this->state = skDISCONNECTED;
	this->totaldata = 0;
}

EzSock::~EzSock()
{
	if (this->check()) {
		close();
	}
}

bool EzSock::check()
{
	return sock > SOCKET_NONE;
}

bool EzSock::create()
{
	return this->create(IPPROTO_TCP, SOCK_STREAM);
}

bool EzSock::create(int Protocol)
{
	switch (Protocol) {
	case IPPROTO_TCP: return this->create(IPPROTO_TCP, SOCK_STREAM);
	case IPPROTO_UDP: return this->create(IPPROTO_UDP, SOCK_DGRAM);
	default:          return this->create(Protocol, SOCK_RAW);
	}
}

bool EzSock::create(int Protocol, int Type)
{
	if (this->check()) {
		return false;
	}

	state = skDISCONNECTED;
	sock = ::socket(AF_INET, Type, Protocol);
	lastCode = sock;

	return sock > SOCKET_NONE;
}

void EzSock::setBlocking(bool blocking)
{
	if (!check()) {
		if (!create()) {
			return;
		}
	}

	u_long nonblocking = (blocking ? 0 : 1);
	ioctlsocket(sock, FIONBIO, &nonblocking);
	this->blocking = blocking;
}

bool EzSock::bind(unsigned short port)
{
	if (!check()) {
		if (!create()) {
			return false;
		}
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	lastCode = ::bind(sock, (struct sockaddr*)&addr, sizeof(addr));

	return !lastCode;
}

bool EzSock::listen()
{
	lastCode = ::listen(sock, MAXCON);
	if (lastCode == SOCKET_ERROR) {
		return false;
	}

	state = skLISTENING;
	this->valid = true;
	return true;
}

bool EzSock::accept(EzSock* socket)
{
	if (!blocking && !canRead()) {
		return false;
	}

	int length = sizeof(socket->addr);
	socket->sock = ::accept(sock, (struct sockaddr*) &socket->addr, (socklen_t*)&length);

	lastCode = socket->sock;
	if (socket->sock == SOCKET_ERROR) {
		return false;
	}

	socket->state = skCONNECTED;
	return true;
}

void EzSock::close()
{
	state = skDISCONNECTED;

#ifdef _MSC_VER
	::closesocket(sock);
#else
	::shutdown(sock, SHUT_RDWR);
	::close(sock);
#endif

	sock = INVALID_SOCKET;
}

long EzSock::uAddr()
{
	return addr.sin_addr.s_addr;
}

int EzSock::connect(const char* host, unsigned short port)
{
	if (!check()) {
		if (!create()) {
			return 1;
		}
	}

	struct hostent* phe;
	phe = gethostbyname(host);
	if (phe == NULL) {
		return 2;
	}

	memcpy(&addr.sin_addr, phe->h_addr, sizeof(struct in_addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		if (this->blocking) {
			return 3;
		}
	}

	state = skCONNECTED;
	this->valid = true;
	return 0;
}

bool EzSock::canRead()
{
	FD_ZERO(&scks);
	FD_SET((unsigned)sock, &scks);

	return select(sock + 1, &scks, NULL, NULL, &times) > 0;
}

bool EzSock::canWrite()
{
	FD_ZERO(&scks);
	FD_SET((unsigned)sock, &scks);

	return select(sock + 1, NULL, &scks, NULL, &times) > 0;
}

bool EzSock::isError()
{
	if (state == skERROR || sock == -1) {
		return true;
	}

	FD_ZERO(&scks);
	FD_SET((unsigned)sock, &scks);

	if (select(sock + 1, NULL, NULL, &scks, &times) >= 0) {
		return false;
	}

	state = skERROR;
	return true;
}

int EzSock::receiveUDP(const void* buffer, int size, sockaddr_in* from)
{
#ifdef _MSC_VER
	int client_length = (int)sizeof(struct sockaddr_in);
#else
	unsigned int client_length = (unsigned int)sizeof(struct sockaddr_in);
#endif
	return recvfrom(this->sock, (char*)buffer, size, 0, (struct sockaddr*)from, &client_length);
}

int EzSock::receive(const void* buffer, int size, int spos)
{
	return recv(this->sock, (char*)buffer + spos, size, 0);
}

int EzSock::sendUDP(const void* buffer, int size, sockaddr_in* to)
{
	return sendto(this->sock, (char*)buffer, size, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in));
}

int EzSock::sendRaw(const void* data, int dataSize)
{
	return send(this->sock, (char*)data, dataSize, 0);
}

#endif
#endif
