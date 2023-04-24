/*
 * constants.h
 *
 *  Created on: 08.05.2018
 *      Author: stoffregenf
 */

#ifndef MAIN_CONSTANTS_H_
#define MAIN_CONSTANTS_H_
#include <string>
namespace constants {
extern int MAX_ITEM_SIZE;
extern int MAX_WIFI_CONNECTION_RETRIES;

struct {
	const char * servicePrefix = "91bad492-b950-4226-aa2b-0000000000";
	const char * registerService = "91bad492-b950-4226-aa2b-ffffffffffff";
	const char * characteristicPrefix = "00000000-b950-4226-aa2b-";
	const int maximumDevices = 3;
} ble;

struct {
	const char * ssid = "MasterDemo";
	const char * password = "12345678";
} wifi;
struct {
//	const std::string url = "http://10.2.7.101:8080/api/";
	const std::string url = "http://192.168.178.21:8080/api/";

} REST;

extern const char * server_ip;
}

#endif /* MAIN_CONSTANTS_H_ */
