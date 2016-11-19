
/*
   Wash U Racing

   This sketch collects data over CAN from the AEM EMS 4 ecu. It then logs that data in CSV format to an sd card.

   Make sure you use the libraries included from the ArduinoCAN folder. The libraries in that folder have
   been modified to work with the AEM ems4 ecu.
*/
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"
#include "PacketSender.h"


const int MESSAGE_ONE = 4294942720;
const int MESSAGE_TWO = 4294942721;
const int MESSAGE_THREE = 4294942722;
const int MESSAGE_FOUR = 4294942723;



const int RLSensorIn = A0;
const int RRSensorIn = A1;

const int rxPinXBee = 12;
const int txPinXBee = 13;

SoftwareSerial XBee_Serial(rxPinXBee,txPinXBee);
PacketSender XBee(XBee_Serial, 9600);

const float RL_SCALE = 0.07458;
const float RR_SCALE = 0.07356;
int valueRL = 0;
int valueRR = 0;
float DispRL; //key: 0x38
float DispRR; //key: 0x39

const int deltaTime = 200;
unsigned long accumulator = 0;
const int chipSelect = 9;
File logFile;
char* filename;
char* finalFileName;

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

double rpm;           //key: 0x30
double load;          //key: 0x31
double throttle;      //key: 0x32
double coolantF;      //key: 0x33
double o2;            //key: 0x34
double vehicleSpeed;  //key: 0x35
byte gear;            //key: 0x36
double volts;         //key: 0x37


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

  filename = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) {
      finalFileName = filename;
      break;
    }
  }

}

void loop() {

  logFile = SD.open(finalFileName, FILE_WRITE);
  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      String dataLine = "";

      if (millis() - accumulator > deltaTime){
        valueRL = analogRead(RLSensorIn);
        valueRR = analogRead(RRSensorIn); 
        DispRL = abs(RL_SCALE*valueRL - 76.3);
        DispRR = abs(RR_SCALE*valueRR - 75.25);

        payload outgoing;
        
        outgoing.floatData = DispRR;
        XBee.sendPayloadTimestamp(outgoing, 0x38);

        outgoing.floatData = DispRL;
        XBee.sendPayloadTimestamp(outgoing, 0x39);
        
        logFile.print(DispRR);
        logFile.print(", ");
        logFile.print(DispRL);
        logFile.print(", ");
        logFile.print(millis()/1000.0);
        logFile.println();

      }
      
      payload outgoing;

      switch (message.id) {

        case MESSAGE_ONE: {

            dataLine = "RPM_LOAD_THROTTLE_COOLANT, ";
            // log rpm
            uint16_t rawRPM = (uint16_t)message.data[0] << 8;
            rawRPM |= message.data[1];
            rpm = rawRPM * RPM_SCALE;
            dataLine = dataLine + rpm + ", ";

            outgoing.floatData = rpm;
            XBee.sendPayloadTimestamp(outgoing, 0x30);

            //log engine load
            uint16_t rawLoad = (uint16_t)message.data[2] << 8;
            rawLoad |= message.data[3];
            load = rawLoad * ENG_LOAD_SCALE;
            dataLine = dataLine + load + ", ";

            outgoing.floatData = load;
            XBee.sendPayloadTimestamp(outgoing, 0x31);

            //log throttle position
            uint16_t rawThrottle = (uint16_t)message.data[4] << 8;
            rawThrottle |= message.data[5];
            throttle = rawThrottle * ENG_THROTTLE_SCALE;
            dataLine = dataLine + throttle + ", ";

            outgoing.floatData = throttle;
            XBee.sendPayloadTimestamp(outgoing, 0x32);

            //log coolant temp
            int8_t coolantC = message.data[7];
            coolantF = ((double)coolantC * 1.8) + 32;
            dataLine = dataLine + coolantF + ", ";

            outgoing.floatData = coolantF;
            XBee.sendPayloadTimestamp(outgoing, 0x33);

            //log time (since arduino started)
            dataLine = dataLine + (millis()/1000.0);
           
            Serial.println(dataLine);
            logFile.println(dataLine);

            break;
          }

        case MESSAGE_FOUR: {
            dataLine = "O2_SPEED_GEAR_VOLTAGE, ";
            //log O2
            uint8_t rawo2 = (uint8_t)message.data[0];
            o2 = rawo2 * O2_SCALE + 0.5;
            dataLine = dataLine + o2 +  ", ";

            outgoing.floatData = o2;
            XBee.sendPayloadTimestamp(outgoing, 0x34);

            uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
            rawSpeed |= message.data[3];
            vehicleSpeed = rawSpeed * SPEED_SCALE;
            dataLine = dataLine + vehicleSpeed + ", ";

            outgoing.floatData = vehicleSpeed;
            XBee.sendPayloadTimestamp(outgoing, 0x35);

            gear = message.data[4];
            dataLine = dataLine + gear + ", ";

            XBee.sendByteTimestamp(gear, 0x36);

            uint16_t rawVolts = (uint16_t)message.data[7] << 8;
            rawVolts |= message.data[8];
            volts = rawVolts * BATT_VOLTAGE_SCALE;
            dataLine = dataLine + volts + ", ";

            outgoing.floatData = volts;
            XBee.sendPayloadTimestamp(outgoing, 0x37);

            
            // log time (since arduino started)
            dataLine = dataLine + (millis()/1000.0);
            Serial.println(dataLine);
            logFile.println(dataLine);
            break;
          }

        default: {
            break;
          }
          
      }
    }
    else {
      Serial.println("No data");
    }   
  }
  logFile.close();
}
