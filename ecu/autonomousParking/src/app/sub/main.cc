/*
 * \brief  Sub - subscribes on topcis and receives messages from server
 * \author Alexander Reisner
 * \date   2017-07-16
 */

/* mosquitto includes */
#include <mosquittopp.h>

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
#include <string>
#include <timer_session/connection.h>

/*pub*/
#include <publisher.h>

float steer, brake, accel, spinVel0, spinVel1, spinVel2, spinVel3, length, width, wheelRadius, gps_x, gps_y, laser0, laser1, laser2, laser3;

Publisher *pub;

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
	sprintf(buffer, "%s,%f", name, value);
	ret = Publisher::publish(NULL, "car-control", strlen(buffer), buffer);
	//PDBG("pub control '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
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
			//PDBG("%s %s", message->topic, message->payload);
			std::string payload = (char*)message->payload;
			const char* name = payload.substr(0, payload.find(";")).c_str();
			//PDBG("name %s", name);
			if(!strcmp(name,"steer"))
			{
				payload.erase(0, payload.find(";")+2);
				steer=atof(payload.substr(0, payload.find(";")).c_str());
				pub->my_publish("0", steer);
			}
			if(!strcmp(name,"brake"))
			{
				payload.erase(0, payload.find(";")+2);
				brake=atof(payload.substr(0, payload.find(";")).c_str());
				pub->my_publish("1", brake);
			}
			if(!strcmp(name,"accel"))
			{
				payload.erase(0, payload.find(";")+2);
				accel=atof(payload.substr(0, payload.find(";")).c_str());
				pub->my_publish("2", accel);
			}
			if(!strcmp(name,"wheel0"))
			{
				payload.erase(0, payload.find(";")+2);
				spinVel0=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"wheel1"))
			{
				payload.erase(0, payload.find(";")+2);
				spinVel1=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"wheel2"))
			{
				payload.erase(0, payload.find(";")+2);
				spinVel2=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"wheel3"))
			{
				payload.erase(0, payload.find(";")+2);
				spinVel3=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"length"))
			{
				payload.erase(0, payload.find(";")+2);
				length=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"width"))
			{
				payload.erase(0, payload.find(";")+2);
				width=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"wheelRadius"))
			{
				payload.erase(0, payload.find(";")+2);
				wheelRadius=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"gps_x"))
			{
				payload.erase(0, payload.find(";")+2);
				gps_x=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"gps_y"))
			{
				payload.erase(0, payload.find(";")+2);
				gps_y=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"laser0"))
			{
				payload.erase(0, payload.find(";")+2);
				laser0=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"laser1"))
			{
				payload.erase(0, payload.find(";")+2);
				laser1=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"laser2"))
			{
				payload.erase(0, payload.find(";")+2);
				laser2=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"laser3"))
			{
				payload.erase(0, payload.find(";")+2);
				laser3=atof(payload.substr(0, payload.find(";")).c_str());
				//char buffer[10000] = { 0 };
				//sprintf(buffer, "%f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f;", steer, brake, accel, spinVel0, spinVel1, spinVel2, spinVel3, length, width, wheelRadius, gps_x, gps_y, laser0, laser1, laser2, laser3);
				//PDBG("%s\n", buffer);
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
			ret = Sub::subscribe(NULL, "state", 0);
			PDBG("Subscribed '%s' successful: %d", "state", MOSQ_ERR_SUCCESS == ret);
			//i++;
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

	Timer::Connection timer;
	timer.msleep(10000);

	/* get config */
	Genode::Xml_node mosquitto = Genode::config()->xml_node().sub_node("mosquitto");

	char ip_addr[16] = {0};
	char port[5] = {0};

	mosquitto.attribute("ip-address").value(ip_addr, sizeof(ip_addr));
	mosquitto.attribute("port").value(port, sizeof(port));

	/* create new mosquitto peer */
	PDBG("Ecu sub init");
	Sub *sub = new Sub("EcuSub", ip_addr, atoi(port));
	PDBG("done");

	PDBG("Ecu pub init");
	pub = new Publisher("EcuPub", ip_addr, atoi(port));
	PDBG("done");

	/* endless loop with auto reconnect */
	pub->loop_start();
	sub->loop_start();

	while(1);
	

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
