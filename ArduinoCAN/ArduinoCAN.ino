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
#include "PacketSender.h"


#include <SPI.h>
#include <Wire.h>

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
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
  B00000000, B00110000 };


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
double vehicleSpeed;
byte gear;
double volts;

boolean warning = false;

#define OVERHEATING 93
#define REDLINE 12000


#define rxPinXBee 2
#define txPinXBee 3

SoftwareSerial XBee(rxPinXBee, txPinXBee);
PacketSender toRadio(XBee);



void setup(){

  rpm = 0;
  load = 0;
  coolant = 0;
  vehicleSpeed = 0;
  gear = 0;
  volts = 0;
  
  Serial.begin(9600);
  //Initialise MCP2515 CAN controller at the specified speed
  if(Canbus.init(CANSPEED_500)){
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Couldn't Init CAN");
  }

  delay(500);

  display.begin();
  display.display();
  delay(1000);

}






void loop(){ 

if (millis() > displayLoopEndTime){
  display.clearDisplay();
  
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.print((long)rpm);
  display.setTextSize(1);
  display.println("rpm");

  
  display.setCursor(0, 20);
  display.println("Gear:");
  display.setCursor(0,28);
  display.setTextSize(2);
  display.println(gear);
  

  display.setCursor(115, 50);
  display.print("C");

  display.drawCircle(110, 50, 2, WHITE);

  if (coolant >= OVERHEATING || rpm >= REDLINE){
    warning =  !(warning);
  }
  else if (warning){
    warning = false;
  }

  if (warning){
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

  if (mcp2515_check_message()){
    if(mcp2515_get_message(&message)){

      switch(message.id){

        case MESSAGE_ONE: {
          uint16_t rawRPM = (uint16_t)message.data[0] << 8;
          rawRPM |= message.data[1];
          rpm = rawRPM * RPM_SCALE;

          toRadio.logRPM(rpm);

          uint16_t rawLoad = (uint16_t)message.data[2] << 8;
          rawLoad |= message.data[3];
          load = rawLoad * ENG_LOAD_SCALE;

          toRadio.logLoad(load);

          coolant = message.data[7];

          toRadio.logCoolant(coolant);
          
          break;
        }

        case MESSAGE_FOUR: {
          uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
          rawSpeed |= message.data[3];
          vehicleSpeed = rawSpeed * SPEED_SCALE;

          toRadio.logSpeed(vehicleSpeed);

          gear = message.data[4];

          toRadio.logGear(gear);

          uint16_t rawVolts = (uint16_t)message.data[7] << 8;
          rawVolts |= message.data[8];
          volts = rawVolts * BATT_VOLTAGE_SCALE;

          toRadio.logVolts(volts);
          break;
        }

         default: {
          break;
         }

      } 
    }
  }
  else{
    Serial.println("No data");   
  }

  
  
  if (millis() > loopEndTime){
    ++coolant;
    ++gear;
    rpm += 500;
    loopEndTime += 1000;
  }



}







