/* mosquitto includes */
#include <mosquittopp.h>

class Publisher : public mosqpp::mosquittopp {
public:
	Publisher(const char* id, const char* host, int port);

	/* connect callback */
	void on_connect(int ret);

	/* publish callback */
	void on_publish(int ret);

	/* log callback */
	void on_log(int ret);

	/* disconnect callback */
	void on_disconnect(int ret);
	
	/* error callback */
	void on_error();

	void my_publish(const char* name, float value);
};
