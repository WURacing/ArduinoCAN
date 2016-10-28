

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

const int chipSelect = 9;
File dataFile;

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

double rpm;
double load;
double throttle;
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

 Serial.print("Initializing SD card...");

 pinMode(chipSelect, OUTPUT);
 
 if (!SD.begin(chipSelect)) {
   
    Serial.println("Card failed, or not present");
    
    // don't do anything more:
    return;
  } 
  Serial.println("card initialized.");

  char file[] = "log00.csv";

  for (uint8_t i = 0; i<100; ++i){
      file[3] = i/10 + '0';
      file[4] = i%10 + '0';

      if(!SD.exists(file)){
        dataFile = SD.open(file, FILE_WRITE);
      }  
  }  
}

void loop() {

  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      String dataLine = "";
     
      switch (message.id) {

        case MESSAGE_ONE: {

            dataLine = "RPM_LOAD_THROTTLE_COOLANT, ";
            // log rpm
            uint16_t rawRPM = (uint16_t)message.data[0] << 8;
            rawRPM |= message.data[1]; 
            rpm = rawRPM * RPM_SCALE;
            dataLine = dataLine + rpm + ", ";

            //log engine load
            uint16_t rawLoad = (uint16_t)message.data[2] << 8;
            rawLoad |= message.data[3];
            load = rawLoad * ENG_LOAD_SCALE;
            dataLine = dataLine + load + ", ";

            //log throttle position
            uint16_t rawThrottle = (uint16_t)message.data[4] << 8;
            rawThrottle |= message.data[5];
            throttle = rawThrottle * ENG_THROTTLE_SCALE;
            dataLine = dataLine + throttle + ", ";

            //log coolant temp
            int8_t coolantC = message.data[7];
            coolantF = ((double)coolantC * 1.8) + 32;     
            dataLine = dataLine + coolantF;

            Serial.println(dataLine);
            dataFile.println(dataLine);
            
            break;
          }

        case MESSAGE_FOUR: {
            dataLine = "O2_SPEED_GEAR_VOLTAGE, ";
            //log O2
            uint8_t rawo2 = (uint8_t)message.data[0];
            o2 = rawo2 * O2_SCALE + 0.5;
            dataLine = dataLine + o2 +  ", ";
            

            uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
            rawSpeed |= message.data[3];
            vehicleSpeed = rawSpeed * SPEED_SCALE;
            dataLine = dataLine + vehicleSpeed + ", ";

            gear = message.data[4];
            dataLine = dataLine + gear + ", ";

            uint16_t rawVolts = (uint16_t)message.data[7] << 8;
            rawVolts |= message.data[8];
            volts = rawVolts * BATT_VOLTAGE_SCALE;
            dataLine = dataLine + volts;

            Serial.println(dataLine);
            dataFile.println(dataLine);      
            break;
          }

        default: {
            break;
      }

      Serial.println();
      dataFile.println();
    }
  }
  else {
    Serial.println("No data");
  }
  dataFile.close();
}
}
