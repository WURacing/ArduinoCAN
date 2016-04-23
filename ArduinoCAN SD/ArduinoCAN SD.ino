#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

#include <SoftwareSerial.h>
#include "PacketSender.h"


#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include "QueueList.h"
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>

//#define OLED_RESET 4
//Adafruit_SSD1306 display(OLED_RESET);

//#define LOGO16_GLCD_HEIGHT 16
//#define LOGO16_GLCD_WIDTH  16
//static const unsigned char PROGMEM logo16_glcd_bmp[] =
/*{ B00000000, B11000000,
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
  B00000000, B00110000 };*/



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

const char* filename = "CANLog.csv";

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

const int rpmDeltaTime = 250;
unsigned long rpmLoopEndTime = 6000;

double rpm;
double load;
int8_t coolant;
double vehicleSpeed;
byte gear;
double volts;


#define rxPinXBee 2
#define txPinXBee 3

SoftwareSerial XBee(rxPinXBee, txPinXBee);
PacketSender toRadio(XBee);

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
    return;
  }
  Serial.println("card initialized.");

  delay(500);

  /*display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    // init done

    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    delay(6000);
    display.clearDisplay();*/

}




void loop() {

  if (millis() > rpmLoopEndTime) {
    /*display.clearDisplay();
    display.setTextSize(4);
    display.setTextColor(WHITE);
    display.setCursor(0,15);
    display.println((int)rpm);
    display.display();
    rpmLoopEndTime += rpmDeltaTime;*/
  }

// open the file. note that only one file can be open at a time,
// so you have to close this one before opening another.
// this opens the file and appends to the end of file
// if the file does not exist, this will create a new file.
  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      switch (message.id) {

      case MESSAGE_ONE: {
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

      case MESSAGE_FOUR: {
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

      default: {
        break;
      }

      }
    }
  }
  uint16_t counter = rpmList.count();
  if (counter > loadList.count()) counter = loadList.count();
  if (counter > coolantList.count()) counter = coolantList.count();
  if (counter > vehicleSpeedList.count()) counter = vehicleSpeedList.count();
  if (counter > gearList.count()) counter = gearList.count();
  if (counter > voltsList.count()) counter = voltsList.count();

  String dataString = "";
  for (int i = 0; i < counter; i++) {
    File dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
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
}

void StringToCard(File dataFile, char* c) {
  if (dataFile) {
    dataFile.print(c);
  }
}

void IntToCard(File dataFile, uint16_t i) {
  char* c = "";
  c += i;
  StringToCard(dataFile, c);
}






