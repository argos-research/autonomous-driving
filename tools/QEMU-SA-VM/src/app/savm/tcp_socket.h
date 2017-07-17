#pragma once

#include <base/printf.h>
#include <cstdint>

extern "C" {
#include <lwip/sockets.h>
}

// Macro to detect connection loss/termination and break out of the loop in that case.
#define NETCHECK_LOOP(x)\
	if(x<1)\
	{ \
		PINF("Connection terminated. Waiting for new connection.\n");\
		break;\
	}



class Tcp_socket
{
public:
	Tcp_socket();

	virtual int connect() = 0;
	virtual void disconnect() = 0;

	virtual ~Tcp_socket()
	{
	}

	// Receive data from the socket and write it into data.
	ssize_t receive_data(void* data, size_t size);

	// convenience function
	ssize_t receiveInt32_t(int32_t& data);

	// Send data from buffer data with size size to the socket.
	ssize_t send_data(void* data, size_t size);

	// convenience function
	ssize_t sendInt32_t(int32_t data);

protected:
	int _target_socket;
	struct sockaddr_in _target_sockaddr_in;
};
