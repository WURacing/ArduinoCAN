#include "PacketSender.h"

PacketSender::PacketSender(SoftwareSerial in) : mySerial(in), mask(0), rpm(0), load(0), coolant(0), speed(0), gear(0), volts(0)  {}

void PacketSender::logRPM(double _rpm){
	this->rpm = _rpm;
	this->mask |= 0x20;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::logLoad(double _load){
	this->load = _load;
	this->mask |= 0x10;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::logCoolant(int8_t _coolant){
	this->coolant = _coolant;
	this->mask |= 0x08;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::logSpeed(double _speed){
	this->speed = _speed;
	this->mask |= 0x04;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::logGear(uint8_t _gear){
	this->gear = _gear;
	this->mask |= 0x02;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::logVolts(double _volts){
	this->volts = _volts;
	this->mask |= 0x01;
	if (mask >= 0x3f){
		this->sendPacket();
	}
}

void PacketSender::sendPacket(){
	this->mask = 0;
	this->mySerial.print(this->rpm);
	this->mySerial.print(',');
	this->mySerial.print(this->load);
	this->mySerial.print(',');
	this->mySerial.print(this->coolant);
	this->mySerial.print(',');
	this->mySerial.print(this->speed);
	this->mySerial.print(',');
	this->mySerial.print(this->gear);
	this->mySerial.print(',');
	this->mySerial.print(this->volts);
	this->mySerial.print(',');

	uint32_t timestamp = millis();
	this->mySerial.println(timestamp);

}
