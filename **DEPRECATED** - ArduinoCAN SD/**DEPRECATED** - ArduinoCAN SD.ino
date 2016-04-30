/*
ArduinoCan_SD.ino

WURacing Electronics Team Spring 2016
Carter Durno, Michael Greer, Thomas Kelly, Evan Simkowitz

The purpose of this code is to read packets coming from our AEM Infinity ECU over the
AEMNet protocol and write it over XBee to a computer.

Be aware that to read the XBee stream, we wrote our own reader on the computer side which
displays our data in real-time and writes it to a CSV. Because our ECU writes data over in
packets, we left most of the sorting to our Python application, which reads the data in
and organizes it before saving it.

Many of the libraries have been adapted to work with AEMNet, which varies slightly from the
CAN-Bus standard.

We have updated the code to store a backup of the data to a Micro-SD card in case the stream
is disrupted.
*/


#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

#include <SoftwareSerial.h>
#include "PacketSender.h"

#include <math.h>

#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <QueueList.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
// Used for software SPI
#define OLED_CLK 13
#define OLED_MOSI 11

// Used for software or hardware SPI
#define OLED_CS 10
#define OLED_DC 8

// Used for I2C or SPI
#define OLED_RESET 9

// software SPI
//Adafruit_SSD1305 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// hardware SPI
Adafruit_SSD1305 display(OLED_DC, OLED_RESET, OLED_CS);
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{	B00000000, B11000000,
	B00000001, B11000000,
	B00000001, B11000000,
	B00000011, B11100000,
	B11110011, B11100000,
	B11111110, B11111000,
	B01111110, B11111111,
	B00110011, B10011111,
	B00011111, B11111100,
	B00001101, B01110000,
	B00011011, B10100000,
	B00111111, B11100000,
	B00111111, B11110000,
	B01111100, B11110000,
	B01110000, B01110000,
	B00000000, B00110000
};


//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif


/* MESSAGE IDS:

MESSAGE_ONE:
bytes 0 and 1 --> engine speed --> scale by .39063 rpm/bit
bytes 2 and 3 --> engine load --> scale by .0015259 % / bit
bytes 4 and 5 --> throttle --> scale by .0015259 % / bit
byte 6 --> air temp --> no scale, 1 bit = 1 deg c
byte 7 --> coolant temp --> no scale, 1 bit = 1 deg c

MESSAGE_TWO:
analog sensors --> 2 bytes for each sensor --> each scaled by .00007782 V/bit

MESSAGE_THREE:
same as message two

MESSAGE_FOUR:
byte 0 --> O2 sensor 1 --> .00390625 lambda / bit
byte 1 --> O2 sensor 2 --> .00390625 lambda / bit
bytes 2 and 3 --> vehicle speed --> .00390625 mph/ bit
byte 4 --> gear calculated --> no scaling
byte 5 --> ign timing --> .35156 deg/bit
bytes 6 and 7 --> battery voltage --> .0002455 V/bit


*/


// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.

// Chip Select pin is tied to pin 9 on the SparkFun CAN-Bus Shield
const int chipSelect = 9;
// filename is for the backup log on the Arduino
const char* filename = "CANLog.csv";

// These correspond to the AEMNet Message IDs. If you don't know what these are,
// AEM provides documentation on their website.
const int MESSAGE_ONE = 4294942720;
const int MESSAGE_TWO = 4294942721;
const int MESSAGE_THREE = 4294942722;
const int MESSAGE_FOUR = 4294942723;

const float RPM_SCALE = .39063;
const float ENG_LOAD_SCALE = .0015259;
const float ENG_THROTTLE_SCALE = .0015259;
const float ANALOG_SCALE = .00007782;
const float O2_SCALE = .00390625;
const float SPEED_SCALE = .00390625;
const float IGN_SCALE = .35156;
const float BATT_VOLTAGE_SCALE = .0002455;

const int displayDeltaTime = 250;
unsigned long displayLoopEndTime = 6000;

unsigned long loopEndTime = 1000;

double rpm;
double load;
int8_t coolant;
double vehicleSpeed;
byte gear;
double volts;

boolean warning = false;

#define OVERHEATING 93
#define REDLINE 12000


#define rxPinXBee 2
#define txPinXBee 3

#define rxPinXBee 2
#define txPinXBee 3

SoftwareSerial XBee(rxPinXBee, txPinXBee);
PacketSender toRadio(XBee);

// For the QueueList buffers, read more below.
QueueList<uint16_t> rpmList;
QueueList<uint16_t> loadList;
QueueList<uint16_t> coolantList;
QueueList<uint16_t> vehicleSpeedList;
QueueList<uint16_t> gearList;
QueueList<uint16_t> voltsList;

void setup() {

	rpm = 0;
	load = 0;
	coolant = 0;
	vehicleSpeed = 0;
	gear = 0;
	volts = 0;

	Serial.begin(9600);
	//Initialise MCP2515 CAN controller at the specified speed
	if (Canbus.init(CANSPEED_500)) {
		Serial.println("CAN Init ok");
	}
	else {
		Serial.println("Couldn't Init CAN");
	}

	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(chipSelect, OUTPUT);

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
	} else
		Serial.println("card initialized.");
	delay(500);

	display.begin();
	display.display();
	delay(1000);

}

void loop() {

	if (millis() > displayLoopEndTime) {
		display.clearDisplay();

		display.setTextSize(2);
		display.setTextColor(WHITE);

		display.setCursor(0, 0);
		display.print((long)rpm);
		display.setTextSize(1);
		display.println("rpm");


		display.setCursor(0, 20);
		display.println("Gear:");
		display.setCursor(0, 28);
		display.setTextSize(2);
		display.println(gear);


		display.setCursor(115, 50);
		display.print("C");

		display.drawCircle(110, 50, 2, WHITE);

		if (coolant >= OVERHEATING || rpm >= REDLINE) {
			warning =  !(warning);
		}
		else if (warning) {
			warning = false;
		}

		if (warning) {
			display.setCursor(0, 50);
			display.setTextSize(2);
			display.print("WARNING");
		}

		int digits = (int) (log10(abs(coolant)));

		display.setTextSize(2);
		display.setCursor(96 - 11 * digits, 50);
		display.print(coolant);

		display.display();



		displayLoopEndTime += displayDeltaTime;
	}

	tCAN message;

	/*
	This section of the code reads messages in from the ECU and writes them to the
	XBee stream, as well as to the QueueLists.
	*/
	if (mcp2515_check_message()) {
		if (mcp2515_get_message(&message)) {

			switch (message.id) {

			case MESSAGE_ONE:
			{
				uint16_t rawRPM = (uint16_t)message.data[0] << 8;
				rawRPM |= message.data[1];
				rpm = rawRPM * RPM_SCALE;

				toRadio.logRPM(rpm);
				rpmList.push(rpm);

				uint16_t rawLoad = (uint16_t)message.data[2] << 8;
				rawLoad |= message.data[3];
				load = rawLoad * ENG_LOAD_SCALE;

				toRadio.logLoad(load);
				loadList.push(load);

				coolant = message.data[7];

				toRadio.logCoolant(coolant);
				coolantList.push(coolant);
				break;
			}

			case MESSAGE_FOUR:
			{
				uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
				rawSpeed |= message.data[3];
				vehicleSpeed = rawSpeed * SPEED_SCALE;

				toRadio.logSpeed(vehicleSpeed);
				vehicleSpeedList.push(vehicleSpeed);

				gear = message.data[4];

				toRadio.logGear(gear);
				gearList.push(gear);

				uint16_t rawVolts = (uint16_t)message.data[7] << 8;
				rawVolts |= message.data[8];
				volts = rawVolts * BATT_VOLTAGE_SCALE;

				toRadio.logVolts(volts);
				voltsList.push(volts);
				break;
			}

			default:
			{
				break;
			}
			}
		}
	}
	else {
		Serial.println("No data");
	}
	/*
	This next part of the code will check all of the QueueLists to see which has the least number
	of entries. It will then record that many entries to the CSV file on the SD card. Because the
	same number of each packet should come in over time, no individual QueueList should get very
	long.
	*/
	uint16_t counter = rpmList.count();
	if (counter > loadList.count()) counter = loadList.count();
	if (counter > coolantList.count()) counter = coolantList.count();
	if (counter > vehicleSpeedList.count()) counter = vehicleSpeedList.count();
	if (counter > gearList.count()) counter = gearList.count();
	if (counter > voltsList.count()) counter = voltsList.count();

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	// this opens the file and appends to the end of file
	// if the file does not exist, this will create a new file.
	String dataString = "";
	for (int i = 0; i < counter; i++) {
		File dataFile = SD.open(filename, FILE_WRITE);
		if (dataFile) {
			dataString += millis();
			dataString += ",";
			dataString += rpmList.pop();
			dataString += ",";
			dataString += loadList.pop();
			dataString += ",";
			dataString += coolantList.pop();
			dataString += ",";
			dataString += vehicleSpeedList.pop();
			dataString += ",";
			dataString += gearList.pop();
			dataString += ",";
			dataString += voltsList.pop();
			dataFile.println(dataString);
			dataFile.close();
		}
	}
	if (millis() > loopEndTime) {
		++coolant;
		++gear;
		rpm += 500;
		loopEndTime += 1000;
	}
}