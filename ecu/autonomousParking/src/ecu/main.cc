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
//#include <publisher.h>
#include <subscriber.h>

/*parking*/
#include "Parking.h"

float steer, brake, accel, spinVel0, spinVel1, spinVel2, spinVel3, length, width, wheelRadius, gps_x, gps_y, laser0, laser1, laser2, laser3, speed, autonomous, steer_max, vel_max=1.5, timestamp, got_go;
bool car_complete, got_length, got_width, got_wheelRadius, got_laser0, got_laser1, got_laser2, got_spinVel, go, got_steerlock;

Publisher *pub;
CarInformation *car;
Parking *parking;

void Publisher::my_publish(const char* name, float value) {
	char buffer[1024] = { 0 };
	sprintf(buffer, "%s,%f", name, value);
	/* int ret =*/ publish(NULL, "car-control", strlen(buffer), buffer);
	//PDBG("pub control '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
}

void Subscriber::on_message(const struct mosquitto_message *message) {
			//PDBG("%s %s", message->topic, message->payload);
			std::string payload = (char*)message->payload;
			const char* name = payload.substr(0, payload.find(";")).c_str();
			if(!strcmp(name,"steer"))
			{
				payload.erase(0, payload.find(";")+2);
				steer=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"brake"))
			{
				payload.erase(0, payload.find(";")+2);
				brake=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"accel"))
			{
				payload.erase(0, payload.find(";")+2);
				accel=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"wheel0"))
			{
				payload.erase(0, payload.find(";")+2);
				spinVel0=atof(payload.substr(0, payload.find(";")).c_str());
				got_spinVel=true;
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
				got_length=true;
			}
			if(!strcmp(name,"width"))
			{	
				payload.erase(0, payload.find(";")+2);
				width=atof(payload.substr(0, payload.find(";")).c_str());
				got_width=true;
			}
			if(!strcmp(name,"wheelRadius"))
			{
				payload.erase(0, payload.find(";")+2);
				wheelRadius=atof(payload.substr(0, payload.find(";")).c_str());
				got_wheelRadius=true;
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
				got_laser0=true;
			}
			if(!strcmp(name,"laser1"))
			{
				payload.erase(0, payload.find(";")+2);
				laser1=atof(payload.substr(0, payload.find(";")).c_str());
				got_laser1=true;
			}
			if(!strcmp(name,"laser2"))
			{
				payload.erase(0, payload.find(";")+2);
				laser2=atof(payload.substr(0, payload.find(";")).c_str());
				got_laser2=true;
			}
			if(!strcmp(name,"steerlock"))
			{
				payload.erase(0, payload.find(";")+2);
				steer_max=atof(payload.substr(0, payload.find(";")).c_str());
				got_steerlock=true;
			}
			if(!strcmp(name,"timestamp"))
			{
				payload.erase(0, payload.find(";")+2);
				timestamp=atof(payload.substr(0, payload.find(";")).c_str());
			}
			if(!strcmp(name,"go"))
			{
				payload.erase(0, payload.find(";")+2);
				got_go=atof(payload.substr(0, payload.find(";")).c_str());
				if(got_go>0)
				{
					go=true;
					pub->my_publish("3", 1);
				}
				else
				{
					go=false;
					pub->my_publish("3", 0);
				}
			}
			if(!car_complete)
			{
				if(got_length&&got_width&&got_wheelRadius&&got_steerlock)
				{
					car=new CarInformation(length,width,wheelRadius,steer_max,vel_max);
					car_complete=true;
					parking = new Parking(*car);
				}
			}
			if(go&&got_laser0&&got_laser1&&got_laser2&&got_spinVel)
			{
				parking->receiveData(laser0, laser1, laser2,spinVel0,timestamp,pub);
				got_laser0=false;
				got_laser1=false;
				got_laser2=false;
				got_spinVel=false;
			}
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
		PDBG("Waiting for 10s");
		Timer::Connection timer;
		timer.msleep(10000);
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

	/* create new mosquitto peer */
	PDBG("Ecu sub init");
	Subscriber *sub = new Subscriber("EcuSub", ip_addr, atoi(port));
	sub->my_subscribe("state");
	PDBG("done");

	PDBG("Ecu pub init");
	pub = new Publisher("EcuPub", ip_addr, atoi(port));
	PDBG("done");

	got_laser0=false;
	got_laser1=false;
	got_laser2=false;
	got_spinVel=false;
	car_complete=false;
	got_length=false;
	got_width=false;
	got_wheelRadius=false;
	go=false;

	PDBG("done");

	/* endless loop with auto reconnect */
	pub->loop_start();
	sub->loop_forever();
	

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
