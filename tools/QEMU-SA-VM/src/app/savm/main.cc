/*
 * \brief  sa/vm - receives tcp/ip packets with protobuf format and publishes mosquitto messages to server
 * \author Alexander Reisner
 * \date   2017-07-16
 */

/* protobuf include */
#include <state.pb.h>
#include <control.pb.h>

#define timeval timeval_linux

#include "publisher.h"
#include "proto_client.h"

/* lwip includes */
extern "C" {
#include <lwip/sockets.h>
}
#include <lwip/genode.h>
#include <nic/packet_allocator.h>

/* genode includes */
#include <base/env.h>
#include <base/printf.h>
#include <util/xml_node.h>
#include <os/config.h>
#include <ram_session/connection.h>
#include <timer_session/connection.h>

/* etc */
#include <cstdio>
#include <cstring>

Publisher::Publisher(const char* id, const char* host, int port) : mosquittopp(id) {
			/* init the library */
			mosqpp::lib_init();

			int keepalive = 60;
			Publisher::connect(host, port, keepalive);
		}

		/* connect callback */
void Publisher::on_connect(int ret) {
			PDBG("Connected with code %d!", ret);
		}

		/* publish callback */
void Publisher::on_publish(int ret) {
			//PDBG("Published with code %d!", ret);
		}


void Publisher::on_log(int ret) {
			PDBG("Log with code %d!", ret);
		}

		/* disconnect callback */
void Publisher::on_disconnect(int ret) {
			PDBG("Disconnected with code %d!", ret);
		}

		/* error callback */
void Publisher::on_error() {
			PDBG("Error!");
		}

void Publisher::my_publish(int id, float value) {
	char buffer[1024] = { 0 };
	int i = 0, ret = -1;
	sprintf(buffer, "%d %f", id, value);
	ret = Publisher::publish(NULL, "state", strlen(buffer), buffer);
	PDBG("state '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
	i++;
}

Proto_client::Proto_client() :
	_listen_socket(0),
	_in_addr{0},
	_target_addr{0}
{
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
			PERR("Reconnecting!");
		}

	PINF("Connected...\n");

}

Proto_client::~Proto_client()
{
	
}

void Proto_client::serve(Publisher *publisher)
{
	Timer::Connection timer;
	timer.msleep(1000);
	int size = 0;
	int id=0;
	float value=0;
	while (true)
	{
		/*NETCHECK_LOOP(receiveInt32_t(size));
		if (size>0)
		{
			PDBG("Ready to receive state.");
			Genode::Ram_dataspace_capability state_ds=Genode::env()->ram_session()->alloc(size);
			protobuf::State* state=Genode::env()->rm_session()->attach(state_ds);
			NETCHECK_LOOP(receive_data(state, size));
			float spinVel=state->wheel(0).spinvel();
			publisher->my_publish(0,0);
			Genode::env()->ram_session()->free(state_ds);
		}
		else
		{
			PWRN("Unknown message: %d", size);
		}*/
	}
}

int Proto_client::connect()
{
	return 0;
}

void Proto_client::disconnect()
{
	lwip_close(_target_socket);
	PERR("Target socket closed.");
	lwip_close(_listen_socket);
	PERR("Server socket closed.");
}	


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
	Proto_client *client;
	PDBG("done");

	/* create new mosquitto peer */
	PDBG("mosquitto init");
	Publisher *publisher = new Publisher("Publisher", ip_addr, atoi(port));
	PDBG("done");

	/* endless loop with auto reconnect */
	publisher->loop_start();

	client->serve(publisher);

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
