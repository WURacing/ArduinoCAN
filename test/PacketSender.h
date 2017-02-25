#ifndef PACKETSENDER_H
#define PACKETSENDER_H

#include <Arduino.h>
#include <SoftwareSerial.h>

union payload {
  double floatData;
  unsigned long intData;
  byte data[4];
};

class PacketSender {
  public:
    PacketSender(SoftwareSerial& stream, int baud);
    void configureMagic(char magic);
    
    void sendPayload(payload msg, char key);
    void sendPayloadTimestamp(payload msg, char key);
    void sendByte(byte msg, char key);
    void sendByteTimestamp(byte msg, char key);

  private:
    SoftwareSerial& stream;
    char magic;
};

class HardwareSender {
  public:
    HardwareSender();
    void configureMagic(char magic);

    void sendPayload(payload msg, char key);
    void sendPayloadTimestamp(payload msg, char key);
    void sendByte(byte msg, char key);
    void sendByteTimestamp(byte msg, char key);

  private:
    char magic;
};

#endif PACKETSENDER_H
