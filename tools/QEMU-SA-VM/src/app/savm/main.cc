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

float steer, brake, accel;

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

void Publisher::my_publish(const char* name, float value) {
	char buffer[1024] = { 0 };
	int i = 0, ret = -1;
	sprintf(buffer, "%s; %f;", name, value);
	ret = Publisher::publish(NULL, "state", strlen(buffer), buffer);
	PDBG("pub state '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
	i++;
}

class Sub : public mosqpp::mosquittopp {
public:
		Sub(const char* id, const char* host, int port) : mosquittopp(id) {
			/* init the library */
			mosqpp::lib_init();

			int keepalive = 60;
			Sub::connect(host, port, keepalive);
		}

		/* connect callback */
		void on_connect(int ret) {
			PDBG("Connected with code %d!", ret);

			Sub::my_subscribe();
		}

		/* publish callback */
		void on_subscribe(int mid, int qos_count, const int* ret) {
			PDBG("Subscribed with codes %d %d %d!", mid, qos_count, *ret);
		}

		void on_message(const struct mosquitto_message *message) {
			PDBG("%s %s", message->topic, message->payload);
			std::string payload = (char*)message->payload;
			const char* name = payload.substr(0, payload.find(";")).c_str();
			//PDBG("name %s", name);
			if(!strcmp(name,"steer"))
			{
				payload.erase(0, payload.find(";")+2);
				steer=atof(payload.substr(0, payload.find(";")).c_str());
				//pub->my_publish("steer", steer);
			}
			if(!strcmp(name,"brake"))
			{
				payload.erase(0, payload.find(";")+2);
				brake=atof(payload.substr(0, payload.find(";")).c_str());
				//pub->my_publish("brake", brake);
			}
			if(!strcmp(name,"accel"))
			{
				payload.erase(0, payload.find(";")+2);
				accel=atof(payload.substr(0, payload.find(";")).c_str());
				//pub->my_publish("accel", accel);
			}
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
		char topic[1024] = { 0 };
		int ret = -1;

		void my_subscribe() {
			ret = Sub::subscribe(NULL, "control", 0);
			PDBG("Subscribed '%s' successful: %d", "state", MOSQ_ERR_SUCCESS == ret);
			//i++;
		};
};

Proto_client::Proto_client() :
	_listen_socket(0),
	_in_addr{0},
	_target_addr{0}
{
	_in_addr.sin_family = AF_INET;
	_in_addr.sin_port = htons(9002);

	enum { BUF_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE * 128 };

	Genode::Xml_node network = Genode::config()->xml_node().sub_node("network");

		char ip_addr[16] = {"131.159.211.132"};
		char subnet[16] = {0};
		char gateway[16] = {0};

		//network.attribute("ip-address").value(ip_addr, sizeof(ip_addr));
		network.attribute("subnet-mask").value(subnet, sizeof(subnet));
		network.attribute("default-gateway").value(gateway, sizeof(gateway));

		_in_addr.sin_addr.s_addr = inet_addr(ip_addr);

		if ((_listen_socket = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			PERR("No socket available!");
			return;
		}
		PDBG("listen socket\n");

		while (lwip_connect(_listen_socket, (struct sockaddr*)&_in_addr, sizeof(_in_addr)))
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
	//timer.msleep(1000);
	int size = 0;
	int id=0;
	float value=0;
	while (true)
	{
		size=0;
		PDBG("loop\n");
		NETCHECK_LOOP(receiveInt32_t(size));
		if (size>0)
		{
			//PDBG("Ready to receive state.");
			//PDBG("Got size %llu\n", ntohl(size));
			Genode::Ram_dataspace_capability state_ds=Genode::env()->ram_session()->alloc(ntohl(size));
			char* foo=Genode::env()->rm_session()->attach(state_ds);
			NETCHECK_LOOP(receive_data(foo, ntohl(size)));
			protobuf::State state;
			state.ParseFromArray(foo,ntohl(size));			
			float steer=state.steer();
			publisher->my_publish("steer",steer);
			float brakeCmd=state.brakecmd();
			publisher->my_publish("brake",brakeCmd);		
			float accelCmd=state.accelcmd();
			publisher->my_publish("accel",accelCmd);
			float spinVel0=state.wheel(0).spinvel();
			publisher->my_publish("wheel0",spinVel0);
			float spinVel1=state.wheel(1).spinvel();
			publisher->my_publish("wheel1",spinVel1);
			float spinVel2=state.wheel(2).spinvel();
			publisher->my_publish("wheel2",spinVel2);
			float spinVel3=state.wheel(3).spinvel();
			publisher->my_publish("wheel3",spinVel3);
			float length=state.specification().length();
			publisher->my_publish("length",length);
			float width=state.specification().width();
			publisher->my_publish("width",width);
			float wheelRadius=state.specification().wheelradius();
			publisher->my_publish("wheelRadius",wheelRadius);
			for(int i=0; i<state.sensor().size(); i++)
			{
				if(state.sensor(i).type()==protobuf::Sensor_SensorType_GPS)
				{
					float gps_x=state.sensor(i).value(0);
					publisher->my_publish("gps_x",gps_x);
					float gps_y=state.sensor(i).value(1);
					publisher->my_publish("gps_y",gps_y);
				}
				else
				{
					float laser=state.sensor(i).value(0);
					char buffer[1024] = { 0 };
					sprintf(buffer, "laser%d", i);
					const char* name=buffer;
					publisher->my_publish(name,laser);
				}
			}
			Genode::env()->ram_session()->free(state_ds);
		}
		else
		{
			PWRN("Unknown message: %d", size);
		}
		std::string foo;
		PDBG("prepare proto");
		protobuf::Control ctrl;
		PDBG("set proto");
		ctrl.set_steer(steer);
		ctrl.set_accelcmd(accel);
		ctrl.set_brakecmd(brake);
		PDBG("start serialize");
		ctrl.SerializeToString(&foo);
		uint32_t length=htonl(foo.size());
		PDBG("control set\n");
		send_data(&length,4);
		send_data((void*)foo.c_str(),foo.size());
		PDBG("data sent to Alex\n");
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
	Proto_client *client = new Proto_client();
	PDBG("done");

	/* create new mosquitto peer */
	PDBG("savm pub init");
	Publisher *pub = new Publisher("SAVMPub", ip_addr, atoi(port));
	PDBG("done");

	PDBG("savm sub init");
	Sub *sub = new Sub("SAVMSub", ip_addr, atoi(port));
	PDBG("done");

	

	/* endless loop with auto reconnect */
	pub->loop_start();
	sub->loop_start();

	client->serve(pub);

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
