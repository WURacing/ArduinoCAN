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
