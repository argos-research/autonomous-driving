#include "subscriber.h"

#include <base/printf.h>

Subscriber::Subscriber(const char* id, const char* host, int port) : mosquittopp(id) {
	/* init the library */
	mosqpp::lib_init();

	int keepalive = 60;
	connect(host, port, keepalive);
}

/* connect callback */
void Subscriber::on_connect(int ret) {
	PDBG("Connected with code %d!", ret);
	//my_subscribe();
}

/* publish callback */
void Subscriber::on_subscribe(int mid, int qos_count, const int* ret) {
	PDBG("Subscribed with codes %d %d %d!", mid, qos_count, *ret);
}

void Subscriber::on_log(int ret) {
	PDBG("Log with code %d!", ret);
}

/* disconnect callback */
void Subscriber::on_disconnect(int ret) {
	PDBG("Disconnected with code %d!", ret);
}

/* error callback */
void Subscriber::on_error() {
	PDBG("Error!");
}
