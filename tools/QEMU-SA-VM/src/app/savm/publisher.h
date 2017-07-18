/* mosquitto includes */
#include <mosquittopp.h>

class Publisher : public mosqpp::mosquittopp {
public:
	Publisher(const char* id, const char* host, int port);

	void on_connect(int ret);

	void on_publish(int ret);

	void on_log(int ret);

	void on_disconnect(int ret);

	void on_error();

	void my_publish(int id, float value);
};
