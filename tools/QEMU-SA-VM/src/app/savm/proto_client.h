#include "tcp_socket.h"

class Proto_client : public Tcp_socket
{
public:
	Proto_client();

	~Proto_client();

	int connect();

	void serve(Publisher *publisher);

	void disconnect();

private:
	int _listen_socket;
	struct sockaddr_in _in_addr;
	sockaddr _target_addr;
};
