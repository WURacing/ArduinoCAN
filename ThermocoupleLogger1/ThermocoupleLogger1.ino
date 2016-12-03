#include <Adafruit_TMP006.h>

/*
name:		ThermocoupleLogger1.ino
version:	v.1.01.00
created:	08/25/2016
author:		Cecil
description: This program is designed to collect and store data from two thermocouple sensors.
			 Requires two MAX31855 thermocouple breakout boards and an SD arduino shield


MAX31855:
        
		/-----------------------\
		|>o  Vin    []  []  [] O|
		|<o  3Vo     _____      |
		|>o  GND    |     |     |
		|<o  DO     |_____|     |
		|>o  CS                 |
		|>o  CLK    [] [][] [] O|
		\-----------------------/
                                                                        
																		       
																			    
*/

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_MAX31855.h>
#include <stdint.h>
#include <math.h>
#include "I2C.h"

//Pin Setup for the Data Logger Shield
#define RTC1  A4
#define RTC2  A5
#define LED1   2		//Yellow LED
#define LED2   3		//Red LED

//Pin Setup for the SD card
#define SDCS  10
#define SDDI  11
#define SDDO  13
#define SDCLK 13

//Pin Setup for the MAX31855 breakout boards
#define TDO    4		//shared between boards
#define TCS1   5
#define TCS2   6
#define TCLK   7 		//shared between boards


//Initialize File and Sensors
File Testlog;
String filename;
Adafruit_MAX31855 Thermo1(TCLK, TCS1, TDO);
Adafruit_MAX31855 Thermo2(TCLK, TCS2, TDO);
RTC_DS1307 RTC;
double time;
double initialtime;
DateTime NOW;


uint8_t addrs[1] = {0x40}; //, 0x41, 0x42, 0x43} ; // I2C address of TMP006, can be 0x40-0x47
uint16_t samples = TMP006_CFG_8SAMPLE; // # of samples per reading, can be 1/2/4/8/16

void setup()
{

	Serial.begin(9600);
	delay(300);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(SDCS, OUTPUT);
  for (auto addr : addrs) {

    config_TMP006(addr, samples);

  }
	//Setup the RTC, blink if failed
	Serial.print("Initializing RTC...");
	Wire.begin();
	if (!RTC.begin()) {
		Blink(LED2, 5);
		Serial.println("FAILED!");
		return;
	}
	else {
		Blink(LED1, 2);
		Serial.println("PASSED!");
		delay(300);
		if (Serial.available()) {
			RTC.adjust(DateTime(__DATE__, __TIME__));
		}
	}

	//Setup the SD Card, blink if failed
	Serial.print("Initializing SD Card...");
	if (!SD.begin(SDCS)) {
		Blink(LED2, 5);
		Serial.println("FAILED!");
		return;
	}
	else {
		Blink(LED1, 2);
		Serial.println("PASSED!");
		delay(500);
	}
	//Set up File, blink if failed
	filename = FileSearch();
	NOW = RTC.now();
	Serial.print("Opening file: " + filename + "...");
	Testlog = SD.open(filename, FILE_WRITE);
	if (Testlog) {
		Blink(LED1, 2);
		Serial.println("PASSED!");
		Serial.println("File: " + filename + "\t\t WashU Racing Temperature Logger");
		 Testlog.println("File: " + filename + "\t\t WashU Racing Temperature Logger");
		Serial.println(Timestamp(NOW));
		 Testlog.println(Timestamp(NOW));
		Serial.println("Time (s)\tTemp01 (C)\tTemp02 (C)");
		 Testlog.println("Time (s)\tTemp01 (C)\tTemp02 (C)");
	}
	else {
		Blink(LED2, 5);
		Serial.println("FAILED!");
		return;
	}
	initialtime = millis();
	Testlog.close();
}

void loop()
{

	Testlog = SD.open(filename, FILE_WRITE);
	delay(50);
	if (Testlog) {
		Serial.print((millis() - initialtime) / 1000.0);
		 Testlog.print((millis() - initialtime) / 1000.0);
		Serial.print("\t");
		 Testlog.print(",");
		 if (Thermo1.readCelsius() != 'nan') {
			 Serial.print(Thermo1.readCelsius());
			 Testlog.print(Thermo1.readCelsius());
		 }
		 else {
			 Serial.print(-25.0);
		 }
		Serial.print("\t");
		 Testlog.print(",");
		 if (Thermo2.readCelsius() != 'nan') {
			 Serial.print(Thermo2.readCelsius());
			 Testlog.print(Thermo2.readCelsius());
		 }
		 else {
			 Serial.println(-25.0);
		 }
	}
 Testlog.print(",");
 for (auto addr : addrs) {
    float object_temp = readObjTempC(addr);
    Testlog.print(object_temp);
    Testlog.print(",");
    Serial.print(" Object Temperature: ");
    Serial.print(object_temp); Serial.println("*C");

  }
  Testlog.println("");
	Testlog.close();
	delay(50);

}

//Blinks a given LED for a given NUMBLINK number of times
void Blink(int LED, int NUMBLINK) {
	for (int i = 0; i < NUMBLINK; i++) {
		digitalWrite(LED, HIGH);
		delay(200);
		digitalWrite(LED, LOW);
		delay(200);
	}
	return;
}

//Searches the SD card to create the next test file name in numerical order
String FileSearch() {
	int index = 0;
	String filename;
	do
	{
		filename = "thermo";
		if (index < 10) {
			filename += "0";
			filename += index;
			filename.concat(".csv");
		}
		else {
			filename += index;
			filename.concat(".csv");
		}
		index += 1;
	} while (SD.exists(filename));
	return(filename);
}

//Creates a timestamp
String Timestamp(DateTime NOW) {
	String timestamp = "";
	timestamp += NOW.hour();
	timestamp += ":";
	timestamp += NOW.minute();
	timestamp += ":";
	timestamp += NOW.second();
	timestamp += "\t";
	timestamp += NOW.day();
	timestamp += "/";
	timestamp += NOW.month();
	timestamp += "/";
	timestamp += NOW.year();
	return(timestamp);
}
