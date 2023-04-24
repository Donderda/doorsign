/*
 * DoorsignDisplay.cpp
 *
 *  Created on: 30.05.2018
 *      Author: stoffregenf
 */

#include "DoorsignDisplay.h"
#define tag "DoorsignDisplay"

#define OFFSET 35
void DoorsignDisplay::stop() {
	this->persist();
}

void DoorsignDisplay::persist() {
	this->epd->end();
}

DoorsignDisplay::DoorsignDisplay(int Pin_PANEL_ON, int Pin_BORDER,
		int Pin_DISCHARGE, int Pin_RESET, int Pin_BUSY, int Pin_MOSI,
		int Pin_MISO, int Pin_SCLK, int Pin_EPD_CS, int Pin_SDA, int Pin_SCL,
		EPD_size EPD_SIZE, int8_t BITS_PER_PIXEL) {
	pinMode(Pin_BUSY, INPUT);
	pinMode(Pin_MISO, INPUT);

	pinMode(Pin_RESET, OUTPUT);
	pinMode(Pin_PANEL_ON, OUTPUT);
	pinMode(Pin_DISCHARGE, OUTPUT);
	pinMode(Pin_BORDER, OUTPUT);
	pinMode(Pin_EPD_CS, OUTPUT);
	pinMode(Pin_MOSI, OUTPUT);
	pinMode(Pin_SCLK, OUTPUT);

	digitalWrite(Pin_RESET, LOW);
	digitalWrite(Pin_PANEL_ON, LOW);
	digitalWrite(Pin_DISCHARGE, LOW);
	digitalWrite(Pin_BORDER, LOW);
	digitalWrite(Pin_EPD_CS, LOW);
	digitalWrite(Pin_MOSI, LOW);
	digitalWrite(Pin_MISO, LOW);
	digitalWrite(Pin_SCLK, LOW);
	lm75a.begin(Pin_SDA, Pin_SCL);
	delay(100);

	this->epd = new EPD_Class(EPD_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT,
			Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_RESET, Pin_BUSY,
			Pin_MOSI, Pin_MISO, Pin_SCLK, Pin_EPD_CS);

	gfx = new MiniGrafx(epd, BITS_PER_PIXEL, palette);

	gfx->init();
	epd->init(); // power up the EPD panel
	if (!epd) {
		ESP_LOGE("EPD", "EPD error = %i", epd->error());
		return;
	}
}

DoorsignDisplay::~DoorsignDisplay() {
	delete gfx;
}

void DoorsignDisplay::showError(std::string message) {
	ESP_LOGE(tag, "Error: %s", message.data());
}

void DoorsignDisplay::drawWiFiConfigInfo(String ssid, String password) {
	epd->setFactor((int) lm75a.getTemperature());
	gfx->init();
	gfx->fillBuffer(0);
	gfx->setColor(1);
	gfx->setFont(ArialMT_Plain_24);
	gfx->setTextAlignment(TEXT_ALIGN_LEFT);
	gfx->drawString(5, 5, "WiFi Configuration");
	gfx->setFont(ArialMT_Plain_16);
	gfx->drawString(10, 35, "Please connect and configure WiFi:");
	String ssidText = "SSID: " + ssid;
	String passwordText = "Password: " + password;
	gfx->drawString(10, 70, ssidText);
	gfx->drawString(10, 100, passwordText);
	gfx->drawString(10, 130, "IP: 192.168.4.1");

	gfx->commit();
	this->persist();
}

void DoorsignDisplay::displaySleep() {
	epd->setFactor((int) lm75a.getTemperature());
	gfx->init();
	gfx->fillBuffer(0);
	gfx->setColor(1);
	gfx->setFont(ArialMT_Plain_24);
	gfx->setTextAlignment(TEXT_ALIGN_LEFT);
	gfx->drawString(20, 30, "Booting");
	gfx->drawString(30, 60, "Please stand by ... ");
	gfx->commit();
}

void DoorsignDisplay::displayState(JsonObject data) {
	int maxWidth = epd->width() - 5;
	epd->setFactor((int) lm75a.getTemperature());

	gfx->init();
	gfx->fillBuffer(0);
	gfx->setColor(1);

	gfx->drawRect(5, 0, maxWidth-5, 25);
	// ToDo:: Printout UI descriptions

	gfx->setFont(ArialMT_Plain_24);
	gfx->setTextAlignment(TEXT_ALIGN_LEFT);
	gfx->drawString(4, 24, data.getString("title").data());

	switch (data.getInt("type")) {
	// ConferenceRoomMode
	case 0: {
		bool hasMore = false;
		JsonArray entries = data.getArray("entries");
		// maximum of 3 entries are displayed
		int length =  entries.size() > 3 ? 3: entries.size();

		for (int i = 0; i < length; i++) {
			if (i == 5) {
				hasMore = true;
				continue;
			}

			JsonObject entry = entries.getObject(i);

			gfx->setFont(ArialMT_Plain_16);
			gfx->drawRect(0, i * OFFSET + 50, 60, 36);
			gfx->drawRect(0, i * OFFSET + 50, maxWidth, 36);

			gfx->drawString(65, i * OFFSET + 50,
					entry.getString("title").data());
			gfx->setFont(ArialMT_Plain_10);
			std::string participants = "";

			JsonArray aParticipants = entry.getArray("participants");
			std::stringstream ss;
			for (int j = 0; j < 2; j++) {
				if (aParticipants.size() > j) {
					ss << aParticipants.getString(j);
					if (j != aParticipants.size() - 1 && j != 3)
						ss << ", ";
				}
			}
			if (aParticipants.size() > 2) {
				ss << aParticipants.size() - 2 << " more";
			}

			gfx->drawString(65, i * OFFSET + 68, ss.str().data());

			gfx->drawLine(0, i * OFFSET + 85, maxWidth, i * OFFSET + 85);
			gfx->drawLine(maxWidth, i * OFFSET + 85, maxWidth, i * OFFSET + 51);
			gfx->setFont(ArialMT_Plain_10);
			std::string time = entry.getString("from").append("-").append(
					entry.getString("to"));
			gfx->drawString(2, i * OFFSET + 67, time.data());

			gfx->drawString(2, i * OFFSET + 52, entry.getString("date").data());
		}

		break;
	}
		// OfficeMode
	case 1: {
		JsonArray entries = data.getArray("entries");
		int rectWidth = maxWidth;
		int items = entries.size();
		if (items == 1) {
			int xStart = 5;
			int height = 124;
			int yStart = OFFSET + 16;

			JsonObject entry = entries.getObject(0);
			gfx->drawRect(xStart, yStart, rectWidth - 5, height);

			gfx->setFont(ArialMT_Plain_24);

			gfx->drawString(5 + xStart, yStart + 3,
					entry.getString("name").data());

			std::string currentRoomName = entry.getString("currentRoomName");
			std::string printRoom = "";
			if (currentRoomName.compare("") != 0) {
				std::string prefix = "Currently in room: ";
				prefix.append(currentRoomName);
				printRoom = prefix;
			} else {
				if (entry.getBoolean("available")) {
					printRoom = "in this room";
				} else {
					printRoom = "not available";
				}
			}
			gfx->setFont(ArialMT_Plain_16);

			gfx->drawStringMaxWidth(5 + xStart, yStart + 2 * OFFSET,
					rectWidth - 11, printRoom.data());

		} else if (items <= 3) {
			for (int i = 0; i < items; i++) {
				int xStart = 5;
				int height = 40;
				int yStart = OFFSET + 16 + i * height;

				JsonObject entry = entries.getObject(i);
				gfx->drawRect(xStart, yStart, rectWidth - 5, height - 1);
				gfx->setFont(Dialog_plain_14);

				gfx->drawString(5 + xStart, yStart + 3,
						entry.getString("name").data());

				std::string currentRoomName = entry.getString(
						"currentRoomName");
				std::string printRoom = "";
				if (currentRoomName.compare("") != 0) {
					std::string prefix = "-> ";
					prefix.append(currentRoomName);
					printRoom = prefix;
				} else {
					if (entry.getBoolean("available")) {
						printRoom = "in this room";
					} else {
						printRoom = "not available";
					}
				}

				gfx->drawString(5 + xStart, yStart + 20, printRoom.data());
			}
		} else if (items == 4) {
			int rows = items + 1 / 2;
			int index = 0;
			for (int row = 0; row < rows; row++) {
				for (int i = 0; i < 2; i++, index++) {
					if (index >= items)
						break;
					int xStart = 5 + rectWidth / 2 * i;
					int height = 60;
					int yStart = OFFSET + 16 + height * row;

					JsonObject entry = entries.getObject(index);
					std::string name = entry.getString("name");
															int strpos = name.find(" ");
															std::string f = name.substr(0, strpos+2);
															f.append(".");
					gfx->drawRect(xStart, yStart, rectWidth / 2 - 5,
							height - 1);
					gfx->setFont(Dialog_plain_12);

					gfx->drawStringMaxWidth(5 + xStart, yStart + 3,
							rectWidth / 2, f.data(),
							false);

					std::string currentRoomName = entry.getString(
							"currentRoomName");
					std::string printRoom = "";
					if (currentRoomName.compare("") != 0) {
						std::string prefix = "-> ";
						prefix.append(currentRoomName);
						printRoom = prefix;
					} else {
						if (entry.getBoolean("available")) {
							printRoom = "in this room";
						} else {
							printRoom = "not available";
						}
					}
					gfx->drawString(5 + xStart, yStart + 28, printRoom.data());
				}
			}
		}

		else if (items <= 6) {
			int rows = items + 1 / 2;
			int index = 0;
			for (int row = 0; row < rows; row++) {
				for (int i = 0; i < 2; i++, index++) {
					if (index >= items)
						break;
					int xStart = 5 + rectWidth / 2 * i;
					int height = 40;
					int yStart = OFFSET + 16 + height * row;

					JsonObject entry = entries.getObject(index);
					std::string name = entry.getString("name");
					int strpos = name.find(" ");
					std::string f = name.substr(0, strpos+2);
					f.append(".");

					Serial.println(f.data());

					gfx->drawRect(xStart, yStart, rectWidth / 2 - 5,
							height - 1);
					gfx->setFont(Dialog_plain_10);
					gfx->drawStringMaxWidth(5 + xStart, yStart + 3,
							rectWidth / 2, f.data(),
							false);

					std::string currentRoomName = entry.getString(
							"currentRoomName");
					std::string printRoom = "";
					if (currentRoomName.compare("") != 0) {
						std::string prefix = "-> ";
						prefix.append(currentRoomName);
						printRoom = prefix;
					} else {
						if (entry.getBoolean("available")) {
							printRoom = "in this room";
						} else {
							printRoom = "not available";
						}
					}
					gfx->drawString(5 + xStart, yStart + 20, printRoom.data());
				}
			}
		} else {
			int rows = items + 1 / 3;
			int index = 0;
			for (int row = 0; row < rows; row++) {
				for (int i = 0; i < 3; i++, index++) {
					if (index >= items)
						break;
					int xStart = 5 + rectWidth / 3 * i;
					int height = 40;
					int yStart = OFFSET + 16 + height * row;

					JsonObject entry = entries.getObject(index);

					std::string name = entry.getString("name");
					int strpos = name.find(" ");
					std::string f = name.substr(0, strpos+2);
					f.append(".");

					gfx->drawRect(xStart, yStart, rectWidth / 3 - 5,
							height - 1);
					gfx->setFont(Dialog_plain_10);
					gfx->drawStringMaxWidth(5 + xStart, yStart + 3,
							rectWidth / 3, f.data(),
							false);

					std::string currentRoomName = entry.getString(
							"currentRoomName");
					std::string printRoom = "";
					if (currentRoomName.compare("") != 0) {
						std::string prefix = "-> ";
						prefix.append(currentRoomName);
						printRoom = prefix;
					} else {
						if (entry.getBoolean("available")) {
							printRoom = "in this room";
						} else {
							printRoom = "not available";
						}
					}
					gfx->drawString(5 + xStart, yStart + 20, printRoom.data());
				}
			}
		}
		break;
	}
	}
	gfx->commit();

}
