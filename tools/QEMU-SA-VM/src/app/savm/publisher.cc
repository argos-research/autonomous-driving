#include "publisher.h"

#include <base/printf.h>

/* etc */
#include <cstdio>
#include <cstring>

Publisher::Publisher(const char* id, const char* host, int port) : mosquittopp(id) {
	/* init the library */
	mosqpp::lib_init();

	int keepalive = 60;
	connect(host, port, keepalive);
}

/* connect callback */
void Publisher::on_connect(int ret) {
	PDBG("Connected with code %d!", ret);
}

/* publish callback */
void Publisher::on_publish(int ret) {
	//PDBG("Published with code %d!", ret);
}

/* on_log callback */
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
