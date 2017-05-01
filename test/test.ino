#include <SoftwareSerial.h>
#include "PacketSender.h"

SoftwareSerial XBee(A3, A4);
HardwareSender tester = HardwareSender();

void setup() {
  Serial.begin(9600);

}

void loop() {

  /*for (byte i = 0; i < 24; ++i){
    payload temp;
    temp.floatData = i * 500.0;
    tester.sendPayloadTimestamp(temp, 0x30);
    delay(100);
    tester.sendByteTimestamp(i, 0x36);
    delay(100);
  }*/

  int reading = analogRead(A0);

  float coeff = reading/1024.0;

  payload temp;
  
  temp.floatData = coeff * 13000;
  tester.sendPayload(temp, 0x30);

  temp.floatData = coeff * 100;
  tester.sendPayload(temp, 0x31);
  tester.sendPayload(temp, 0x32);

  temp.floatData = coeff * 150;
  tester.sendPayload(temp, 0x33);

  temp.floatData = coeff * 80;
  tester.sendPayload(temp, 0x34);

  temp.floatData = coeff * 90;
  tester.sendPayload(temp, 0x35);

  byte tempByte = (byte)(coeff * 11);
  tester.sendByte(tempByte, 0x36);

  temp.floatData = coeff * 12;
  //tester.sendPayload(temp, 0x37);

  temp.floatData = coeff * 120;
  //tester.sendPayload(temp, 0x38);
  //tester.sendPayload(temp, 0x39);

  delay(20);
}
