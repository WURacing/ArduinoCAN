#include <SoftwareSerial.h>
#include "PacketSender.h"

HardwareSender tester = HardwareSender();

void setup() {
  Serial.begin(9600);

}

void loop() {

  for (byte i = 0; i < 10; ++i){
    payload temp;
    temp.floatData = i * 500.0;
    tester.sendPayloadTimestamp(temp, 0x30);
    delay(500);
    tester.sendByteTimestamp(i, 0x36);
    delay(500);
  }

}
