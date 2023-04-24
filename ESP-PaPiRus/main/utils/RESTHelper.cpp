#include "RESTHelper.h"
#define tag "RESTHelper"

std::string RESTHelper::post(std::string path, const char *data,
		long &responseCode) {
	return this->request(Method::POST, path, responseCode, data);
}

std::string RESTHelper::get(std::string path, long &responseCode) {
	return this->request(Method::GET, path, responseCode);
}

std::string RESTHelper::put(std::string path, const char *data,
		long &responseCode) {
	return this->request(Method::PUT, path, responseCode, data);
}

std::string RESTHelper::del(std::string path, long &responseCode) {
	return this->request(Method::DELETE, path, responseCode);
}

std::string RESTHelper::request(Method method, std::string path,
		long &responseCode, const char * data) {
	ESP_LOGI(tag, "------");
	ESP_LOGI(tag, "path: %s", path.data());
	ESP_LOGI(tag, "data: %s", data);
	HTTPClient http;

	String url = String(path.data());
	String payload = "";

	http.begin(url);
	http.addHeader("Content-Type", "application/json");

	switch (method) {
	case Method::GET:
		ESP_LOGI(tag, "Method: GET");
		responseCode = http.GET();
		break;
	case Method::POST:
		ESP_LOGI(tag, "Method: POST");
		responseCode = http.POST(data);
		break;
	case Method::PUT:
		ESP_LOGI(tag, "Method: PUT");
		responseCode = http.PUT(data);
		break;
	case Method::DELETE:
		ESP_LOGI(tag, "Method: DELETE");
		String payload = data;
		responseCode = http.sendRequest("DELETE", payload);
		break;
	}

	// httpCode will be negative on error
	if (responseCode > 0) {
		// file found at server
		payload = http.getString();
	} else {
		ESP_LOGE(tag, "Service not available: %s", path.data());
		//esp_restart();
		return "{}";
	}
	ESP_LOGI(tag, "------");

	char cPayload[payload.length()+1];
	payload.toCharArray(cPayload, payload.length()+1);
	return std::string(cPayload);
}

JsonArray RESTHelper::JSONArrayFromURI(std::string uri, Method method,
		std::string payload) {
	std::string completeURL;
	completeURL.assign(constants::REST.url).append(uri);
	std::string data = "{\"success\": false}";
	long responseCode = 0;
	data = this->request(method, completeURL, responseCode, payload.data());
	return JSON::parseArray(data);
}

JsonObject RESTHelper::JSONObjectFromURI(std::string uri, Method method,
		std::string payload) {
	std::string completeURL;
	completeURL.assign(constants::REST.url).append(uri);

	ESP_LOGD(tag, "JSONObjectFromURI: URL: %s; Payload: %s", completeURL.data(), payload.data());

	std::string data = "{\"success\": false}";
	long responseCode = 501;

	data = this->request(method, completeURL, responseCode, payload.data());

	ESP_LOGD("JSONObjectFromURI", "Data: %s, ReturnCode: %ld", data.data(), responseCode);
	if (responseCode < 0) {
		data = "{\"success\": false}";
	}

	JsonObject returnObject = JSON::parseObject(data);
	returnObject.setInt("responseCode", responseCode);
	return returnObject;
}
