/*
 * \brief  sa/vm - receives tcp/ip packets with protobuf format and publishes mosquitto messages to server
 * \author Alexander Reisner
 * \date   2017-07-16
 */

/* mosquitto includes */
#include <mosquittopp.h>
#include "tcp_socket.h"

/* genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <util/xml_node.h>
#include <os/config.h>

/* lwip includes */
extern "C" {
#include <lwip/sockets.h>
}
#include <lwip/genode.h>
#include <nic/packet_allocator.h>

/* etc */
#include <cstdio>
#include <cstring>

int _listen_socket;
struct sockaddr_in _in_addr;
sockaddr _target_addr;

class Proto_server : public Tcp_socket
{
public:
	Proto_server();

	~Proto_server();

	int connect();

	void serve();

	void disconnect();

private:
	int _listen_socket;
	struct sockaddr_in _in_addr;
	sockaddr _target_addr;
};

Proto_server::Proto_server() :
	_listen_socket(0),
	_in_addr{0},
	_target_addr{0}
{
	//lwip_tcpip_init();

	_in_addr.sin_family = AF_INET;
	_in_addr.sin_port = htons(3001);

	enum { BUF_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE * 128 };

	Genode::Xml_node network = Genode::config()->xml_node().sub_node("network");

		char ip_addr[16] = {0};
		char subnet[16] = {0};
		char gateway[16] = {0};

		network.attribute("ip-address").value(ip_addr, sizeof(ip_addr));
		network.attribute("subnet-mask").value(subnet, sizeof(subnet));
		network.attribute("default-gateway").value(gateway, sizeof(gateway));

		_in_addr.sin_addr.s_addr = inet_addr(ip_addr);

		if ((_listen_socket = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			PERR("No socket available!");
			return;
		}
		PDBG("listen socket\n");

		if (lwip_connect(_listen_socket, (struct sockaddr*)&_in_addr, sizeof(_in_addr)))
		{
			PERR("Connection failed!");
		}

		/*if (lwip_bind(_listen_socket, (struct sockaddr*)&_in_addr, sizeof(_in_addr)))
		{
			PERR("Bind failed!");
			return;
		}
		PDBG("lwip bind\n");
		if (lwip_listen(_listen_socket, 5))
		{
			PERR("Listen failed!");
			return;
		}*/
	PINF("Connected...\n");

}

Proto_server::~Proto_server()
{
	disconnect();
}

int Proto_server::connect()
{
	socklen_t len = sizeof(_target_addr);
	_target_socket = lwip_accept(_listen_socket, &_target_addr, &len);
	if (_target_socket < 0)
	{
		PWRN("Invalid socket from accept!");
		return _target_socket;
	}
	sockaddr_in* target_in_addr = (sockaddr_in*)&_target_addr;
	PINF("Got connection from %s", inet_ntoa(target_in_addr));
	return _target_socket;
}

void Proto_server::serve()
{
	int message = 0;
	while (true)
	{
		NETCHECK_LOOP(receiveInt32_t(message));
		if (message == 0)
		{
			PDBG("Ready to receive task description.");
		}
		else
		{
			PWRN("Unknown message: %d", message);
		}
	}
}

void Proto_server::disconnect()
{
	lwip_close(_target_socket);
	PERR("Target socket closed.");
	lwip_close(_listen_socket);
	PERR("Server socket closed.");
}	


class Publisher : public mosqpp::mosquittopp {
public:
		Publisher(const char* id, const char* host, int port) : mosquittopp(id) {
			/* init the library */
			mosqpp::lib_init();

			int keepalive = 60;
			Publisher::connect(host, port, keepalive);
		}

		/* connect callback */
		void on_connect(int ret) {
			PDBG("Connected with code %d!", ret);

			Publisher::my_publish();
		}

		/* publish callback */
		void on_publish(int ret) {
			PDBG("Published with code %d!", ret);

			Publisher::my_publish();
		}


		void on_log(int ret) {
			PDBG("Log with code %d!", ret);
		}

		/* disconnect callback */
		void on_disconnect(int ret) {
			PDBG("Disconnected with code %d!", ret);
		}

		/* error callback */
		void on_error() {
			PDBG("Error!");
		}


private:
		char buffer[1024] = { 0 };
		int i = 0, ret = -1;

		void my_publish() {
			sprintf(buffer, "Hello World: %d", i);
			ret = Publisher::publish(NULL, "Publisher", strlen(buffer), buffer);
			PDBG("Publish '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
			i++;
		};
};

int main(int argc, char* argv[]) {
	//lwip_tcpip_init(); /* causes freeze, code works fine without it */

	enum { BUF_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE * 128 };

	Genode::Xml_node network = Genode::config()->xml_node().sub_node("network");

	if (network.attribute_value<bool>("dhcp", true)) {
		PDBG("DHCP network...");
		if (lwip_nic_init(0,
		                  0,
		                  0,
		                  BUF_SIZE,
		                  BUF_SIZE)) {
			PERR("lwip init failed!");
			return 1;
		}
		PDBG("done");
	} else {
		PDBG("manual network...");
		char ip_addr[16] = {0};
		char subnet[16] = {0};
		char gateway[16] = {0};

		network.attribute("ip-address").value(ip_addr, sizeof(ip_addr));
		network.attribute("subnet-mask").value(subnet, sizeof(subnet));
		network.attribute("default-gateway").value(gateway, sizeof(gateway));

		if (lwip_nic_init(inet_addr(ip_addr),
		                  inet_addr(subnet),
		                  inet_addr(gateway),
		                  BUF_SIZE,
		                  BUF_SIZE)) {
			PERR("lwip init failed!");
			return 1;
		}

	PDBG("done");
	}

	/* get config */
	Genode::Xml_node mosquitto = Genode::config()->xml_node().sub_node("mosquitto");

	char ip_addr[16] = {0};
	char port[5] = {0};

	mosquitto.attribute("ip-address").value(ip_addr, sizeof(ip_addr));
	mosquitto.attribute("port").value(port, sizeof(port));

	PDBG("protobuf init");
	Proto_server client;
	PDBG("done");

	/* create new mosquitto peer */
	PDBG("mosquitto init");
	class Publisher *publisher = new Publisher("Publisher", ip_addr, atoi(port));
	PDBG("done");

	/* endless loop with auto reconnect */
	publisher->loop_start();

	PDBG("Blub\n");

	while(1);

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
