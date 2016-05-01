#include <Wire.h>
#include <SPI.h>
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

#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

#include <math.h>

#include <SoftwareSerial.h>
//#include "PacketSender.h"


#include <SPI.h>
#include <Wire.h>

#define LOGO16_GLCD_HEIGHT 64
#define LOGO16_GLCD_WIDTH  128
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
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
double o2;
double vehicleSpeed;
byte gear;
double volts;

#define RPM_INDEX 0
#define LOAD_INDEX 1
#define COOLANT_INDEX 2
#define O2_INDEX 0
#define SPEED_INDEX 3
#define GEAR_INDEX 4
#define VOLTS_INDEX 5

double packet[6];

boolean warning = false;

#define OVERHEATING 93
#define REDLINE 12000


#define rxPinXBee 2
#define txPinXBee 6

#define RPM_MAX 12000
#define RPM_MIN 0

SoftwareSerial XBee(rxPinXBee, txPinXBee);
//PacketSender toRadio(XBee);

int SER_Pin = 4;   //pin 14 on the 75HC595
int RCLK_Pin = 3;  //pin 12 on the 75HC595
int SRCLK_Pin = 5; //pin 11 on the 75HC595

//How many of the shift registers - change this
#define number_of_74hc595s 3

//do not touch
#define numOfRegisterPins number_of_74hc595s * 8

boolean registers[numOfRegisterPins];

void setup() {

  XBee.begin(9600);
  
  rpm = 0;
  load = 0;
  coolant = 0;
  vehicleSpeed = 0;
  gear = 0;
  volts = 0;
  o2 = 0;

  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);


  //reset all register pins
  clearRegisters();
  writeRegisters();

  Serial.begin(9600);
  //Initialise MCP2515 CAN controller at the specified speed
  if (Canbus.init(CANSPEED_500)) {
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Couldn't Init CAN");
  }

  delay(500);

  display.begin();
  display.setRotation(2);
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

    setLights();
    writePacket();

    displayLoopEndTime += displayDeltaTime;
  }

  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      switch (message.id) {

      case MESSAGE_ONE: {
        uint16_t rawRPM = (uint16_t)message.data[0] << 8;
        rawRPM |= message.data[1];
        rpm = rawRPM * RPM_SCALE;

        packet[0] = rpm;
        //toRadio.logRPM(rpm);

        uint16_t rawLoad = (uint16_t)message.data[2] << 8;
        rawLoad |= message.data[3];
        load = rawLoad * ENG_LOAD_SCALE;

        packet[1] = load;
        //toRadio.logLoad(load);

        coolant = message.data[7];

        packet[2] = (double)coolant;
        //toRadio.logCoolant(coolant);

        break;
      }

      case MESSAGE_FOUR: {
        uint8_t rawo2 = (uint8_t)message.data[0];
        o2 = rawo2 * O2_SCALE;
        
        uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
        rawSpeed |= message.data[3];
        vehicleSpeed = rawSpeed * SPEED_SCALE;

        packet[3] = vehicleSpeed;
        //toRadio.logSpeed(vehicleSpeed);

        gear = message.data[4];

        packet[4] = (double)gear;
        //toRadio.logGear(gear);

        uint16_t rawVolts = (uint16_t)message.data[7] << 8;
        rawVolts |= message.data[8];
        volts = rawVolts * BATT_VOLTAGE_SCALE;

        packet[5] = volts;
        //toRadio.logVolts(volts);
        break;
      }

      default: {
        break;
      }

      }
    }
  }
  else {
    Serial.println("No data");
  }



  if (millis() > loopEndTime) {
    ++coolant;
    ++gear;
    rpm += 500;
    loopEndTime += 1000;

    packet[COOLANT_INDEX] = coolant;
    packet[GEAR_INDEX] = gear;
    packet[RPM_INDEX] = rpm;
    
    if (rpm > RPM_MAX+1000) {
      rpm -= RPM_MAX;
      gear = 0;
      coolant = 0;
    }
  }



}

void clearRegisters() {
  for (int i = numOfRegisterPins - 1; i >=  0; i--) {
    registers[i] = LOW;
  }
  writeRegisters();
}


//Set and display registers
//Only call AFTER all values are set how you would like (slow otherwise)
void writeRegisters() {

  digitalWrite(RCLK_Pin, LOW);

  for (int i = numOfRegisterPins - 1; i >=  0; i--) {
    digitalWrite(SRCLK_Pin, LOW);

    int val = registers[i];

    digitalWrite(SER_Pin, val);
    digitalWrite(SRCLK_Pin, HIGH);

  }
  digitalWrite(RCLK_Pin, HIGH);

}

//set an individual pin HIGH or LOW
void setRegisterPin(int index, int value) {
  registers[index] = value;
}

//Set the lights according to the RPM value
void setLights() {

  clearRegisters();

  int temp = map(rpm, 0, RPM_MAX, 0, 10);

  for (int i = 0; i < temp; i++) {
    setRegisterPin(i, HIGH);
    setRegisterPin(19 - i, HIGH);
    writeRegisters();
  }
}

void writePacket(){
  for (int i = 0; i < 6; ++i){
    XBee.print(packet[i]);
    XBee.print(',');
  }
  XBee.println(millis());
}






