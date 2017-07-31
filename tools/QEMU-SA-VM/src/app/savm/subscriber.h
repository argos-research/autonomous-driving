/* mosquitto includes */
#include <mosquittopp.h>

class Subscriber : public mosqpp::mosquittopp {
public:
	Subscriber(const char* id, const char* host, int port);

	/* connect callback */
	void on_connect(int ret);

	/* subscribe callback */
	void on_subscribe(int mid, int qos_count, const int* ret);

	/* message callback */
	void on_message(const struct mosquitto_message *message);

	/* log callback */
	void on_log(int ret);

	/* disconnect callback */
	void on_disconnect(int ret);

	/* error callback */
	void on_error();

	void my_subscribe(const char* name);
};
