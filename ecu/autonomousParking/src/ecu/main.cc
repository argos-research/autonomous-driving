/*
 * \brief  ECU - Receives messages from mosquitto server, processes parking manuver and publishes car control messages
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

/* Float variables that are updated on realtime by on message calls
   Needed to feed the parking algorithm with up to date values */
float steer, brake, accel, spinVel0, spinVel1, spinVel2, spinVel3, length, width, wheelRadius, gps_x, gps_y, laser0, laser1, laser2, laser3, speed, autonomous, steer_max, vel_max=1.5, timestamp, got_go;

/* Boolean variables that are initialized false and set true, if component received a message via mosquitto
   Set to false again, after the corresponding message was used */
bool car_complete, got_length, got_width, got_wheelRadius, got_laser0, got_laser1, got_laser2, got_spinVel, go, got_steerlock;

/* Publisher pointer is needed to give it to the parking algorithm, which is then able to publish car control messages */
Publisher *pub;

/* The car information object is constructed once all needed information is in place and is then handed over to the paking object later on */
CarInformation *car;

/* The parking object is called every time all needed information arrived at the ECU to perform the next step of the parking manuver */
Parking *parking;

void Publisher::my_publish(const char* name, float value) {
	char buffer[1024] = { 0 };
	sprintf(buffer, "%s,%f", name, value);
	/* int ret =*/ publish(NULL, "car-control", strlen(buffer), buffer);
	//PDBG("pub control '%s' successful: %d", buffer, MOSQ_ERR_SUCCESS == ret);
}

/* Message callback of mosquitto, executed every time a message on the subscribed topic arrives */
void Subscriber::on_message(const struct mosquitto_message *message) {
	//PDBG("%s %s", message->topic, message->payload);
	std::string payload = (char*)message->payload;
	
	/* take first part of message until ";" and compare it to messages from protobuf
	   save second part of message into corresponding value
	   and set got_something boolean to true, if value is needed elsewhere */
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
		/* protobuf publishes a flot for go,
		   which has to become a boolean here */
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

	/* if car information was already constructed, do not do it again */
	if(!car_complete)
	{
		if(got_length&&got_width&&got_wheelRadius&&got_steerlock)
		{
			car=new CarInformation(length,width,wheelRadius,steer_max,vel_max);
			car_complete=true;
			parking = new Parking(*car);
		}
	}

	/* if all information necessary for next parking calculation arrived
	   and autonomous parking is desired, let the algorithm do its thing */
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

	/* network initialization... */

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
		/* dhcp assignement takes some time... */
		PDBG("Waiting 10s for ip assignement");
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

	/* create ECU subscriber */
	PDBG("Ecu sub init");
	Subscriber *sub = new Subscriber("EcuSub", ip_addr, atoi(port));
	sub->my_subscribe("state");
	PDBG("done");

	/* create ECU publisher */
	PDBG("Ecu pub init");
	pub = new Publisher("EcuPub", ip_addr, atoi(port));
	PDBG("done");

	/* initiate any value with false, you never know... */
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
	/* loop_start creates a thread and makes it possible to execute code afterwards
	   loop_forever creates a thread and lets no code run afterwards */

	/* cleanup */
	mosqpp::lib_cleanup();
	return 0;
}
