#include "PacketSender.h"

PacketSender::PacketSender(SoftwareSerial& stream, int baud) : stream(stream) {
  this->stream.begin(baud);
  this->magic = '!';
}

void PacketSender::configureMagic(char magic){
  this->magic = magic;
}

void PacketSender::sendPayload(payload msg, char key){
  this->stream.write(this->magic);
  this->stream.write(key);

  this->stream.write(msg.data[3]);
  this->stream.write(msg.data[2]);
  this->stream.write(msg.data[1]);
  this->stream.write(msg.data[0]);
}

void PacketSender::sendPayloadTimestamp(payload msg, char key){
  this->stream.write(this->magic);
  this->stream.write(key);

  payload timestamp;
  timestamp.intData = millis();
  this->stream.write(timestamp.data[3]);
  this->stream.write(timestamp.data[2]);
  this->stream.write(timestamp.data[1]);
  this->stream.write(timestamp.data[0]);

  this->stream.write(msg.data[3]);
  this->stream.write(msg.data[2]);
  this->stream.write(msg.data[1]);
  this->stream.write(msg.data[0]);
}


void PacketSender::sendByte(byte msg, char key){
  this->stream.write(this->magic);
  this->stream.write(key);

  this->stream.write(msg);
}

void PacketSender::sendByteTimestamp(byte msg, char key){
  this->stream.write(this->magic);
  this->stream.write(key);

  payload timestamp;
  timestamp.intData = millis();
  this->stream.write(timestamp.data[3]);
  this->stream.write(timestamp.data[2]);
  this->stream.write(timestamp.data[1]);
  this->stream.write(timestamp.data[0]);

  this->stream.write(msg);
}




HardwareSender::HardwareSender(){
  this->magic = '!';
}

void HardwareSender::configureMagic(char magic){
  this->magic = magic;
}

void HardwareSender::sendPayload(payload msg, char key){
  Serial.write(this->magic);
  Serial.write(key);

  Serial.write(msg.data[3]);
  Serial.write(msg.data[2]);
  Serial.write(msg.data[1]);
  Serial.write(msg.data[0]);
}

void HardwareSender::sendPayloadTimestamp(payload msg, char key){
  Serial.write(this->magic);
  Serial.write(key);

  payload timestamp;
  timestamp.intData = millis();
  Serial.write(timestamp.data[3]);
  Serial.write(timestamp.data[2]);
  Serial.write(timestamp.data[1]);
  Serial.write(timestamp.data[0]);

  Serial.write(msg.data[3]);
  Serial.write(msg.data[2]);
  Serial.write(msg.data[1]);
  Serial.write(msg.data[0]);
}


void HardwareSender::sendByte(byte msg, char key){
  Serial.write(this->magic);
  Serial.write(key);

  Serial.write(msg);
}

void HardwareSender::sendByteTimestamp(byte msg, char key){
  Serial.write(this->magic);
  Serial.write(key);

  payload timestamp;
  timestamp.intData = millis();
  Serial.write(timestamp.data[3]);
  Serial.write(timestamp.data[2]);
  Serial.write(timestamp.data[1]);
  Serial.write(timestamp.data[0]);

  Serial.write(msg);
}

