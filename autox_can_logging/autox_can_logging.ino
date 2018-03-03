
/*
   Wash U Racing

   This sketch collects data over CAN from the AEM EMS 4 ecu. It then logs that data in CSV format to an sd card.

   Make sure you use the libraries included from the ArduinoCAN folder. The libraries in that folder have
   been modified to work with the AEM ems4 ecu.
*/
#include <Wire.h>
#include <SD.h>
#include <mcp_can.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_GPS.h>
#include "defaults.h"
#include "global.h"
#include "stdlib.h"
#include "PacketSender.h"

const int MESSAGE_ONE = 2180030467;
const int MESSAGE_TWO = 2180030466;
const int MESSAGE_THREE = 2180030465;
const int MESSAGE_FOUR = 2180030464;
// 5 & 6 not managed by ECU
const int MESSAGE_FIVE = 0xFFFF9E4C;
const int MESSAGE_SIX = 0xFFFFD824;

//CAN Variables Setup
long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
// char msgString[128];

#define CAN0_INT 4
MCP_CAN CAN0(10);

SoftwareSerial gps(7, 6);
Adafruit_GPS GPS(&gps);


const int RLSensorIn = A0;
const int RRSensorIn = A1;

const int rxPinXBee = A3;
const int txPinXBee = A4;

const int frontBrake = A5;
const int backBrake = A2;

//const int rxPinDisplay = 10;
//const int txPinDisplay = 11;

SoftwareSerial XBee_Serial(rxPinXBee,txPinXBee);
PacketSender XBee(XBee_Serial, 9600);
HardwareSender Display = HardwareSender();


const float RL_SCALE = 0.07458;
const float RR_SCALE = 0.07356;
const float RL_OFFSET = 76.3;
const float RR_OFFSET = 75.25;
int valueRL = 0;
int valueRR = 0;
int valueFB = 0;
int valueBB = 0;
float DispRL; //key: 0x38
float DispRR; //key: 0x39
float DispFB;
float DispBB;

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

  //INIT Can Communications
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
      //Serial.println("MCP2515 Initialized Successfully!");
  } else {
      //Serial.println("Error Initializing MCP2515...");
  }

  CAN0.setMode(MCP_NORMAL); // send acknowledgements to recieved data
  pinMode(CAN0_INT, INPUT);

  GPS.begin(9600);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  delay(500);

  /*Serial.print("Initializing SD card...");

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
  }*/

}

SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
}

void loop() {
//  char c = GPS.read();
//  Serial.write(c);
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
    payload outgoing;
    if (!GPS.parse(GPS.lastNMEA())) {
      outgoing.intData = 0;
      XBee.sendPayloadTimestamp(outgoing, 0x42);
      Display.sendPayloadTimestamp(outgoing, 0x42);
      outgoing.intData = 0;
      XBee.sendPayloadTimestamp(outgoing, 0x43);
      Display.sendPayloadTimestamp(outgoing, 0x43);
    } else {
      outgoing.intData = GPS.latitude_fixed;
      XBee.sendPayloadTimestamp(outgoing, 0x42);
      Display.sendPayloadTimestamp(outgoing, 0x42);
      outgoing.intData = GPS.longitude_fixed;
      XBee.sendPayloadTimestamp(outgoing, 0x43);
      Display.sendPayloadTimestamp(outgoing, 0x43);
    }
  }
  
  //logFile = SD.open(finalFileName, FILE_WRITE);
  if (millis() - accumulator > deltaTime){
    valueRL = analogRead(RLSensorIn);
    valueRR = analogRead(RRSensorIn); 
    DispRL = abs(RL_SCALE*valueRL - RL_OFFSET);
    DispRR = abs(RR_SCALE*valueRR - RR_OFFSET); 
    valueFB = analogRead(frontBrake);
    valueBB = analogRead(backBrake);
    DispFB = (valueFB*6.106) - 621.63;
    DispBB = (valueBB*6.106) - 621.63;
  
    payload outgoing;
    
    outgoing.floatData = DispRR;
    XBee.sendPayloadTimestamp(outgoing, 0x38);
  
    outgoing.floatData = DispRL;
    XBee.sendPayloadTimestamp(outgoing, 0x39);

    outgoing.floatData = DispFB;
    XBee.sendPayloadTimestamp(outgoing, 0x40);

    outgoing.floatData = DispBB;
    XBee.sendPayloadTimestamp(outgoing, 0x41);

    accumulator += deltaTime;
    
  }

  if (!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

      String dataLine = "";
      
      payload outgoing;

      switch (rxId & 0x1FFFFFFF) {

        case MESSAGE_ONE: {

            //dataLine = "RPM_LOAD_THROTTLE_COOLANT, ";
            // log rpm
            uint16_t rawRPM = (uint16_t)rxBuf[0] << 8;
            rawRPM |= rxBuf[1];
            rpm = rawRPM * RPM_SCALE;
            //dataLine = dataLine + rpm + ", ";

            outgoing.floatData = rpm;
            XBee.sendPayloadTimestamp(outgoing, 0x30);
            Display.sendPayload(outgoing, 0x30);

            //log engine load
            uint16_t rawLoad = (uint16_t)rxBuf[2] << 8;
            rawLoad |= rxBuf[3];
            load = 100.0 - (rawLoad * ENG_LOAD_SCALE);
            //dataLine = dataLine + load + ", ";

            outgoing.floatData = load;
            XBee.sendPayloadTimestamp(outgoing, 0x31);
            //Display.sendPayload(outgoing, 0x31);

            //log throttle position
            uint16_t rawThrottle = (uint16_t)rxBuf[4] << 8;
            rawThrottle |= rxBuf[5];
            throttle = rawThrottle * ENG_THROTTLE_SCALE;
            //dataLine = dataLine + throttle + ", ";

            outgoing.floatData = throttle;
            XBee.sendPayloadTimestamp(outgoing, 0x32);
            //Display.sendPayload(outgoing, 0x32);

            //log coolant temp
            int8_t coolantC = rxBuf[7];
            coolantF = ((double)coolantC * 1.8) + 32;
            //dataLine = dataLine + coolantF + ", ";

            outgoing.floatData = coolantF;
            XBee.sendPayloadTimestamp(outgoing, 0x33);
            Display.sendPayload(outgoing, 0x33);

            //log time (since arduino started)
            //dataLine = dataLine + (millis()/1000.0);
           
            //Serial.println(dataLine);
            //logFile.println(dataLine);

            break;
          }

        case MESSAGE_FOUR: {
            //dataLine = "O2_SPEED_GEAR_VOLTAGE, ";
            //log O2
            uint8_t rawo2 = (uint8_t)rxBuf[0];
            o2 = rawo2 * O2_SCALE + 0.5;
            //dataLine = dataLine + o2 +  ", ";

            outgoing.floatData = o2;
            XBee.sendPayloadTimestamp(outgoing, 0x34);

            uint16_t rawSpeed = (uint16_t)rxBuf[2] << 8;
            rawSpeed |= rxBuf[3];
            vehicleSpeed = rawSpeed * SPEED_SCALE;
            //dataLine = dataLine + vehicleSpeed + ", ";

            outgoing.floatData = vehicleSpeed;
            XBee.sendPayloadTimestamp(outgoing, 0x35);
            //Display.sendPayload(outgoing, 0x35);

            gear = rxBuf[4];
            //dataLine = dataLine + gear + ", ";

            XBee.sendByteTimestamp(gear, 0x36);
            //Display.sendByte(gear, 0x36);

            uint16_t rawVolts = (uint16_t)rxBuf[7] << 8;
            rawVolts |= rxBuf[8];
            volts = rawVolts * BATT_VOLTAGE_SCALE;
            //dataLine = dataLine + volts + ", ";

            outgoing.floatData = volts;
            XBee.sendPayloadTimestamp(outgoing, 0x37);
            Display.sendPayload(outgoing, 0x37);

            
            // log time (since arduino started)
            //dataLine = dataLine + (millis()/1000.0);
            //Serial.println(dataLine);
            //logFile.println(dataLine);
            break;
          }

        case MESSAGE_FIVE:
        case MESSAGE_SIX: {
          break;
        }

        default: {
            break;
          }
          
      }
    }
  //logFile.close();
}

/*int test_rpm() {
  float rpm = 8000;
  Display.sendPayload(rpm, 0x30);
}

int test_temp() {
  float temp = 100;
  Display.sendPayload(temp, 0x33);
}

int test_gear() {
  Display.sendByte(0x05, 0x36);
}*/


