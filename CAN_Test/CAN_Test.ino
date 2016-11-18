#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

const int MESSAGE_ONE = 4294942720;
const int MESSAGE_TWO = 4294942721;
const int MESSAGE_THREE = 4294942722;
const int MESSAGE_FOUR = 4294942723;

// AEMNet sends all values as integers, which must be converted
// with a predetermined scale. These scales were obtained from
// AEM's documentation
const float RPM_SCALE = .39063;
const float ENG_LOAD_SCALE = .0015259;
const float ENG_THROTTLE_SCALE = .0015259;
const float ANALOG_SCALE = .00007782;
const float O2_SCALE = .00390625;
const float SPEED_SCALE = .00390625;
const float IGN_SCALE = .35156;
const float BATT_VOLTAGE_SCALE = .0002455;



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

int8_t coolantC;

double rpm;
double load;
double coolantF;
double o2;
double vehicleSpeed;
byte gear;
double volts;


void setup() {
  Serial.begin(9600);

  if (Canbus.init(CANSPEED_500)) {
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Couldn't Init CAN");
  }

  delay(500);
  
}

void loop() {

  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {
      Serial.println("We found data");
      Serial.print(message.id,HEX);
      for (int i = 0; i < 8; ++i){
        Serial.print(message.data[i],HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
  else {
    Serial.println("No data");
  }
}
