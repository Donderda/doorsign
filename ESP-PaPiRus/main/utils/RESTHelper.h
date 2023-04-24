#ifndef MAIN_UTILS_RESTHelper_H_
#define MAIN_UTILS_RESTHelper_H_
#include "sdkconfig.h"
#include <string>
#include <HTTPClient.h>
#include <esp_log.h>
#include "JSON.h"
#include "constants.h"

enum Method {
	GET, PUT, POST, DELETE
};

class RESTHelper {
public:
	~RESTHelper() {
	}
	std::string post(std::string path, const char *data, long &responseCode);
	//,			curl_slist *header = NULL);
	std::string get(std::string path, long &responseCode);
	//, curl_slist *header =	NULL);
	std::string put(std::string path, const char *data, long &responseCode);
	//,			curl_slist *header = NULL);
	std::string del(std::string path, long &responseCode);
	//, curl_slist *header =	NULL);
	JsonObject JSONObjectFromURI(std::string uri, Method method,
			std::string payload = "");
	JsonArray JSONArrayFromURI(std::string uri, Method method,
			std::string payload = "");
	RESTHelper() {
	}
private:
	std::string request(Method method, std::string path, long &responseCode,
	/*curl_slist *header = NULL,*/const char * data = "");
};

#endif /* MAIN_UTILS_RESTHelper_H_ */

