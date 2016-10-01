#ifndef PACKETSENDER_H
#define PACKETSENDER_H

#include "Arduino.h"
#include <SoftwareSerial.h>

class PacketSender {
private:

	SoftwareSerial mySerial;

	uint8_t mask;
	double rpm;
	double load;
	int8_t coolant;
	double speed;
	uint8_t gear;
	double volts;

	void sendPacket();

public:
	PacketSender(SoftwareSerial in);

	void logRPM(double _rpm);
	void logLoad(double _load);
	void logCoolant(int8_t _coolant);
	void logSpeed(double _speed);
	void logGear(uint8_t _gear);
	void logVolts(double _volts);
};

#endif

