#include <Arduino.h>
#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <algorithm>

#include "../components/arduino/libraries/WiFi/src/WiFi.h"
#include <WiFiMulti.h>
#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "BLEUUID.h"
#include "BLEAddress.h"
#include "Task.h"
#include "BLEUtils.h"
#include "DoorsignDisplay.h"
#include "Helpers.h"
#include "RESTHelper.h"
#include "esp_phy_init.h"
#include "esp_err.h"
#include "rom/md5_hash.h"
#include "nvs.h"

// Display pins
#define Pin_WAKEUP 		   2
#define Pin_BUSY          25
#define Pin_RESET         26
#define Pin_PANEL_ON      27
#define Pin_DISCHARGE     14
#define Pin_BORDER        12
// Buttons
#define Pin_BTN_1		  32
#define Pin_BTN_2		  4
#define Pin_BTN_3		  36
#define Pin_BTN_4		  34
// SPI
#define Pin_MOSI 		  21
#define Pin_MISO 		  22
#define Pin_SCLK          23
#define Pin_EPD_CS        33
//I2C
#define Pin_SDA           5
#define Pin_SCL           18

#define EPD_SIZE EPD_2_7
#define BITS_PER_PIXEL 1

#define GPIO_INPUT_PIN_SEL  ((1ULL<<Pin_BTN_1))
#define ESP_INTR_FLAG_DEFAULT 0

int timestamp = 0;

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

static RESTHelper helper;

// The remote service we wish to connect to.
static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-ffffffffffff");
static BLEUUID announceUUID("91bad492-b950-4226-aa2b-000000000000");

// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");
static RTC_DATA_ATTR std::vector<BLEAddress*> blacklist;

static void updateDisplay();
void gotoSleep(int seconds);
static DoorsignDisplay* display = nullptr;
static bool scanInProgress = false;
static bool finished = false;

typedef unsigned char digest_t[16];

// save gateway mac and bootcount to deepsleep save ram.
RTC_DATA_ATTR char gateway[18] = { '0', '0', ':', '0', '0', ':', '0', '0', ':',
		'0', '0', ':', '0', '0', ':', '0', '0' };
RTC_DATA_ATTR int bootCount = 0;



void resetGateway() {
	char tmp[18] = { '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':',
			'0', '0', ':', '0', '0' };
	std::copy(tmp, tmp + 18, gateway);
}

void WiFiEvent(WiFiEvent_t event) {
	Serial.printf("[WiFi-event] event: %d\n", event);

	switch (event) {
	case SYSTEM_EVENT_STA_GOT_IP:
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		Serial.println("WiFi lost connection");

		//  This is here to force the ESP32 to reset the WiFi and initialise correctly.
		WiFi.disconnect(true);
		delay(1000);
		WiFi.mode(WIFI_STA);
		delay(1000);
		// End silly stuff !!!

		WiFi.begin(constants::wifi.ssid, constants::wifi.password);
		delay(5000);
		break;
	default:
		Serial.print("Status: ");
		Serial.println(event);
		break;
	}
}

void initWifi() {
	ESP_LOGI(tag, "Connecting to WiFi: %s", constants::wifi.ssid);

	// We start by connecting to a WiFi network
	//DO NOT TOUCH
	//  This is here to force the ESP32 to reset the WiFi and initialise correctly.
	Serial.print("WIFI status = ");
	Serial.println(WiFi.getMode());
	WiFi.disconnect(true);
	delay(1000);
	WiFi.mode(WIFI_STA);
	delay(1000);
	Serial.print("WIFI status = ");
	Serial.println(WiFi.getMode());
	// End silly stuff !!!
	delay(500);

	WiFi.onEvent(WiFiEvent);
	WiFi.begin(constants::wifi.ssid, constants::wifi.password);

	Serial.println();
	Serial.println();
	Serial.print("Wait for WiFi... ");

	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
	}
	delay(500);
}

void initDisplay();

class MyClient: public Task {
	void run(void* data) {
		scanInProgress = true;
		std::stringstream ss;
		ss << constants::ble.characteristicPrefix
				<< BLEDevice::getAddress().toString();

		std::string id = ss.str();
		Helpers::find_and_replace(id, ":", "");

		BLEUUID myCharUUID(id);

		ESP_LOGI(LOG_TAG, "find service / characteristic UUID: %s",id.data());

		BLEAddress* pAddress = (BLEAddress*) data;
		BLEClient* pClient = BLEDevice::createClient();

		// Connect to the remote BLE Server.
		if (!pClient->connect(*pAddress)) {
			ESP_LOGE(LOG_TAG, "Client was not able to connect to Gateway");
			scanInProgress = false;
			finished = true;
			gotoSleep(2);
			return;
		}ESP_LOGI(LOG_TAG, "Connected");

		auto services = pClient->getServices();
		auto it = services->begin();
		ESP_LOGI("SERVICES", "Found %i services", services->size());

		// Iterate over the map using Iterator till end.
		bool foundService = false;
		BLERemoteService * service = nullptr;
		while (it != services->end()) {
			auto currentService = it->second;
			auto strong = it->first;

			ESP_LOGI("SERVICES", "Service: %s", strong.data());

			if (strong == constants::ble.registerService) {
				service = currentService;
			}

			if (strong == id) {
				foundService = true;
				service = currentService;
				ESP_LOGI("SERVICES", "found service");
				int n = pAddress->toString().length();
				strcpy(gateway, pAddress->toString().c_str());
				gotoSleep(5);
				break;
			}

			it++;
		}

		if (!foundService) {
			ESP_LOGI("SERVICES", "Service not found. Trying to register service...@%s", constants::ble.registerService);
			if (service) {
				BLERemoteCharacteristic* pRemoteCharacteristic =
						service->getCharacteristic(
								constants::ble.registerService);

				std::ostringstream stringStream;
				stringStream << "NEW" << BLEDevice::getAddress().toString();
				ESP_LOGI("SERVICES", ">> %s", stringStream.str().data());
				pRemoteCharacteristic->writeValue(stringStream.str());
				ESP_LOGE("Service", "found service @%s",
						pAddress->toString().data());
				int n = pAddress->toString().length();

				// copying the contents of the
				// string to char array
				strcpy(gateway, pAddress->toString().c_str());
				pClient->disconnect();
				gotoSleep(5);
			} else {
				ESP_LOGE("SERVICES", "No free slots :(");
				blacklist.push_back(pAddress);
				pClient->disconnect();
			}
		}

		ESP_LOGI(tag, "Service: %s", gateway);

		scanInProgress = false;
	} // run
};
// MyClient

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.*/

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGI(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());

		bool blacklistedService = false;
		for (auto const& value : blacklist) {
			if (value->toString() == advertisedDevice.getAddress().toString()) {
				blacklistedService = true;
				break;
			}
		}
		if (!blacklistedService) {
			if (advertisedDevice.haveServiceUUID()
					&& advertisedDevice.isAdvertisingService(announceUUID)) {
				advertisedDevice.getScan()->stop();

				ESP_LOGI(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
				MyClient* pMyClient = new MyClient();
				pMyClient->setStackSize(18000);
				pMyClient->start(
						new BLEAddress(
								*advertisedDevice.getAddress().getNative()));
			}
		} else {
			ESP_LOGI(LOG_TAG, "Device is blacklisted: %s", advertisedDevice.getAddress().toString().data());
		}
	} // onResult
};
// MyAdvertisedDeviceCallbacks
void checkforData() {
	std::string gatewayMac(gateway);
	if (gatewayMac.compare("00:00:00:00:00:00") == 0) {

		BLEScan *pBLEScan = BLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(
				new MyAdvertisedDeviceCallbacks());
		pBLEScan->setActiveScan(true);
		finished = false;
		scanInProgress = false;

		int scans = 3;

		while (!finished && scans != 0) {
			ESP_LOGI("checkForData", "scans = %i; finished = %i; scanInProgress = %i", scans, finished, scanInProgress);
			delay(100);
			if (!scanInProgress) {
				pBLEScan->start(15);
				scans--;
				ESP_LOGI("checkForData", "scan tries remaining: %i", scans);
			} else {
				delay(1000);
			}
		}

		if (scans == 0) {
			display->showError("No Gateway found.");
		}
	} else {
		ESP_LOGI("checkForData", "Trying to connect to gateway: %s", gatewayMac.data());

		BLEAddress* pAddress = new BLEAddress(gatewayMac.data());
		BLEClient* pClient = BLEDevice::createClient();
		if (!pClient->connect(*pAddress)) {
			ESP_LOGE("checkForData", "Cannot connect to: %s",
					gatewayMac.data());
			resetGateway();

			gotoSleep(2);
			return;
		}
		std::stringstream ss;
		ss << constants::ble.characteristicPrefix
				<< BLEDevice::getAddress().toString();

		std::string id = ss.str();
		Helpers::find_and_replace(id, ":", "");
		BLERemoteService * service = nullptr;

		auto services = pClient->getServices();
		auto it = services->begin();

		while (it != services->end()) {
			auto currentService = it->second;
			auto strong = it->first;

			ESP_LOGI("SERVICES", "Service: %s", strong.data());

			if (strong == id) {
				BLERemoteCharacteristic* pRemoteCharacteristic =
						currentService->getCharacteristic(id);

				ESP_LOGI(tag, "charValue: %s", pRemoteCharacteristic->readValue().data());
				if (pRemoteCharacteristic->readValue().compare("true") == 0) {
					initWifi();
					updateDisplay();
				}
				return;
				break;
			}

			it++;
		}
	}

	/**/
}

static void updateDisplay() {
	initDisplay();

	std::string fullPath = "smartdoor/";
	fullPath.append(BLEDevice::getAddress().toString());
	JsonObject myJSON = helper.JSONObjectFromURI(fullPath, Method::GET);
	Serial.println(myJSON.toString().data());
	scanInProgress = false;
	if (myJSON.getInt("responseCode") == 404) {
		resetGateway();
		gotoSleep(2);
		delay(5000);
	}

	if (myJSON.getInt("responseCode") != 200)
		return;
	timestamp = myJSON.getInt("timestamp");
	display->displayState(myJSON);
}

/*
 Method to print the reason by which ESP32
 has been awaken from sleep
 */
void handleWakeup() {
	esp_sleep_wakeup_cause_t wakeup_reason;
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch (wakeup_reason) {
	case 1:
		Serial.println("Wakeup caused by external signal using RTC_IO");
		break;
	case 2:
		Serial.println("Wakeup caused by external signal using RTC_CNTL");
		break;
	case 3:
		Serial.println("Wakeup caused by timer");
		Serial.println(bootCount);
		if(bootCount == 1){
			Serial.println("First Update by force!");
			initWifi();
			updateDisplay();
			bootCount++;
			gotoSleep(30);
		}
		break;
	case 4:
		Serial.println("Wakeup caused by touchpad");
		break;
	case 5:
		Serial.println("Wakeup caused by ULP program");
		break;
	default:
		Serial.println("Wakeup was not caused by deep sleep");
		display->displaySleep();
		bootCount++;
		break;
	}
}
/**
 * put the ESP into deepsleep
 * @seconds the time in seconds
 */
void gotoSleep(int seconds) {
// TODO: Set Timer
// TODO: Wake Up On trigger
	ESP_LOGI("gotoSleep", "Sleeping for %i seconds", seconds);
	display->persist();
	delete (display);

	//esp_bt_controller_disable();
	//esp_bluedroid_disable();
	WiFi.disconnect(true);

	delay(200);
	esp_sleep_enable_timer_wakeup(seconds * uS_TO_S_FACTOR);
	esp_deep_sleep_start();
}

void initDisplay() {
	if (!display) {
		display = new DoorsignDisplay(
		Pin_PANEL_ON,
		Pin_BORDER,
		Pin_DISCHARGE,
		Pin_RESET,
		Pin_BUSY,
		Pin_MOSI,
		Pin_MISO,
		Pin_SCLK,
		Pin_EPD_CS,
		Pin_SDA,
		Pin_SCL,
		EPD_SIZE,
		BITS_PER_PIXEL);
	}
}

void setup() {
	Serial.begin(115200);
	BLEDevice::init("");
	initDisplay();

	std::string s(gateway);
	Serial.println(s.data());
	//initWifi();
	//updateDisplay();
	handleWakeup();

	checkforData();
	ESP_LOGI("app_main", "check for data finished");
	gotoSleep(30);
}

void loop() {
	delay(1000);
}
