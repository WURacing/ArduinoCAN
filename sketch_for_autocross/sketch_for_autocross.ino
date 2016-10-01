//Temporary sketch for data logging during autocross testing on Oct 2nd

#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

#include <math.h>

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

int8_t coolantC;

double rpm;
double load;
double coolantF;
double o2;
double vehicleSpeed;
byte gear;
double volts;

union {
      char bVal[4];
      float fVal;
    } volatile wheelSpeed;

const int RLSensorIn = A0; 
const int RRSensorIn = A1; 
int valueRL = 0;
int valueRR = 0;
float DispRL; 
float DispRR;


//define the I2C slave address for sensor 1
#define I2C_SLAVE1_ADDR 0x04

#define RPM_INDEX 0
#define LOAD_INDEX 1
#define COOLANT_INDEX 2
#define O2_INDEX 3
#define GEAR_INDEX 4
#define VOLTS_INDEX 5

#define NUM_OUTPUTS 6

double packet[7];

// Chip Select pin is tied to pin 9 on the SparkFun CAN-Bus Shield
const int chipSelect = 9;  
#define FILTER_COUNTS 8
float last_vals[FILTER_COUNTS];
uint32_t counter = 0;

unsigned int deltaTime = 250;
unsigned long accumulator = deltaTime;

File dataFile;

void setup() {
  Serial.begin(9600);

  Serial.print("Initializing SD card...");
 

  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }

  Serial.println("card initialized.");
  Wire.begin();
  //initialize wheelSpeed float value
  wheelSpeed.fVal = 0.0;

  dataFile = SD.open("datalog.csv", FILE_WRITE);

  if (!dataFile){
    Serial.println("Couldn't open the csv file");
  }
  else{
    dataFile.println("Starting new run.");
    dataFile.println("time (ms), speed (mph), Engine Speed (RPM), Engine Load (%), Coolant Temp (F), O2 Level (%), Gear (calculated), Battery (V)");
  }
  
}

void loop() {
  
    valueRL = analogRead(RLSensorIn);
    valueRR = analogRead(RRSensorIn); 
    DispRL = abs(0.07458*valueRL - 76.3);
    DispRR = abs(0.07356*valueRR - 75.25);
    
    
    //output also on Serial monitor for debugging
    #ifdef DEBUG
    Serial.print(timeStamp);
    Serial.print(" ms");
    Serial.print(",");
    #endif
    //request 4 bytes of the float representation from the slave
    for(int i = 0; i < 4; i++)
    {
      Wire.requestFrom(I2C_SLAVE1_ADDR, 1);
      while(Wire.available())
      {
        wheelSpeed.bVal[i] = Wire.read();
      }
    }
    last_vals[counter % FILTER_COUNTS] = wheelSpeed.fVal;
    counter++;
    float avg = 0;
    for (int i = 0; i < FILTER_COUNTS; ++i) {
      avg += last_vals[i];
    }
    avg = avg/(float)FILTER_COUNTS;
    #ifdef DEBUG
    Serial.println(avg);
    #endif

  
  
  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      Serial.println("We found data");

      switch (message.id) {

        case MESSAGE_ONE: {
            uint16_t rawRPM = (uint16_t)message.data[0] << 8;
            rawRPM |= message.data[1];
            rpm = rawRPM * RPM_SCALE;

            packet[RPM_INDEX] = rpm;

            uint16_t rawLoad = (uint16_t)message.data[2] << 8;
            rawLoad |= message.data[3];
            load = rawLoad * ENG_LOAD_SCALE;

            packet[LOAD_INDEX] = load;

            coolantC = message.data[7];

            coolantF = ((double)coolantC * 1.8) + 32;

            packet[COOLANT_INDEX] = coolantF;

            break;
          }

        case MESSAGE_FOUR: {
            uint8_t rawo2 = (uint8_t)message.data[0];
            o2 = rawo2 * O2_SCALE + 0.5;

            packet[O2_INDEX] = o2;

            /*uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
            rawSpeed |= message.data[3];
            vehicleSpeed = rawSpeed * SPEED_SCALE;

            packet[SPEED_INDEX] = vehicleSpeed;*/

            gear = message.data[4];

            packet[GEAR_INDEX] = (double)gear;

            uint16_t rawVolts = (uint16_t)message.data[7] << 8;
            rawVolts |= message.data[8];
            volts = rawVolts * BATT_VOLTAGE_SCALE;

            packet[VOLTS_INDEX] = volts;
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

  if (millis() - accumulator > deltaTime){
    if(dataFile){
      dataFile.print(millis());
      dataFile.print(", ");
      dataFile.print(avg);
      dataFile.print(", ");
      for (int i = 0; i < NUM_OUTPUTS; ++i){
        dataFile.print(packet[i]);
        dataFile.print(", ");
      }
    }
  }

}
