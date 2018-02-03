// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();

int offset = 2000;

MCP_CAN CAN0(10);     // Set CS to pin 10

void setup()
{
  Serial.begin(115200);

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  mma.setRange(MMA8451_RANGE_2_G);
}

byte data[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};


void loop()
{
  sensors_event_t event; 
  mma.getEvent(&event);

  //min value of event.accleration * 100 is -2000 so offset is 2000
  int accX = (event.acceleration.x * 100) + offset;
  int accY = (event.acceleration.y * 100) + offset;
  int accZ = (event.acceleration.z * 100) + offset;

  // max value for acc is 4000 so numbers are represented in 12 bits
  data[0] = accX >> 4;
  data[1] = (0xF0 & accY) | (0xF & accX);
  data[2] = (0XF0 & (accY >> 4)) | (0xF & accY);
  data[3] = accZ >> 4;
  data[4] = 0xF & accZ;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;
  
  // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
  byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, data);
  if(sndStat == CAN_OK){
    Serial.println("Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Message...");
  }
  delay(20);   // send data per 100ms
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
