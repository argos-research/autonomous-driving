#include "tcp_socket.h"


#include <errno.h>

Tcp_socket::Tcp_socket() :
	_target_socket{0},
	_target_sockaddr_in{0}
{
}

// Receive data from the socket and write it into data.
ssize_t Tcp_socket::receive_data(void* data, size_t size)
{
	ssize_t result = 0;
	ssize_t position = 0;
	// Because read() might actually read less than size bytes
	// before it returns, we call it in a loop
	// until size bytes have been read.
	do
	{
		result = lwip_read(_target_socket, (char*) data + position, size - position);
		if (result < 1)
		{
			return -errno;
		}
		position += result;

	} while ((size_t) position < size);

	return position;
}

// convenience function
ssize_t Tcp_socket::receiveInt32_t(int32_t& data)
{
	return receive_data(&data, sizeof(data));
}

// Send data from buffer data with size size to the socket.
ssize_t Tcp_socket::send_data(void* data, size_t size)
{
	ssize_t result = 0;
	ssize_t position = 0;

	// Because write() might actually write less than size bytes
	// before it returns, we call it in a loop
	// until size bytes have been written.
	do
	{
		result = lwip_write(_target_socket, (char*) data + position, size - position);
		if (result < 1)
			return -errno;
		position += result;

	} while ((size_t) position < size);

	return position;
}

// convenience function
ssize_t Tcp_socket::sendInt32_t(int32_t data)
{
	return send_data(&data, sizeof(data));
}
