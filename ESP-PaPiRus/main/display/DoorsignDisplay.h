/*
 * DoorsignDisplay.h
 *
 *  Created on: 30.05.2018
 *      Author: stoffregenf
 */

#ifndef MAIN_DISPLAY_DOORSIGNDISPLAY_H_
#define MAIN_DISPLAY_DOORSIGNDISPLAY_H_

#include <inttypes.h>
#include <ctype.h>
#include <Arduino.h>
#include <SPI.h>
#include <sstream>
#include "MiniGrafx.h"
#include "DisplayDriver.h"
#include "EPaperPervasive.h"
#include "M2M_LM75A.h"
#include "font.h"
#include "image.h"

#include "JSON.h"

#define SCREEN_WIDTH 264
#define SCREEN_HEIGHT 176

class DoorsignDisplay {
public:
	DoorsignDisplay(int Pin_PANEL_ON, int Pin_BORDER,
			int Pin_DISCHARGE, int Pin_RESET, int Pin_BUSY, int Pin_MOSI, int Pin_MISO, int Pin_SCLK,
			int Pin_EPD_CS, int Pin_SDA, int Pin_SCL, EPD_size EPD_SIZE, int8_t BITS_PER_PIXEL);
	void stop();
	void displayState(JsonObject data);
	void showError(std::string message);
	void displaySleep();
	virtual ~DoorsignDisplay();
	void persist();
	void drawWiFiConfigInfo(String ssid, String password);
private:
	EPD_Class * epd = nullptr;
	MiniGrafx * gfx = nullptr;
	uint16_t palette[2] = { 0, 1 };
	M2M_LM75A lm75a;
};

#endif /* MAIN_DISPLAY_DOORSIGNDISPLAY_H_ */
