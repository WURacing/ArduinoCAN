CAN Communicator
==============

Written in C and C++. Requires cmake, avr-gcc, and avrdude.

Features
--
* Read and parse ECU data from CAN bus.
* Read sensor data from analog inputs
* Read GPS location and time information
* Send this data over serial UART (to an XBee, USB to a PC, RasPi, etc)

Communication
--

This project communicates with the following pins:

UART (XBee or PC):
* D0 (RX)
* D1 (TX)

Software UART (GPS module):
* C3 (RX)
* C4 (TX)

CAN:
* B2 (CS)
* B3 (MOSI)
* B4 (MISO)
* B5 (SCK)

Sensor Inputs - analog:
* C0
* C1
* C2
* C5

When built in Debug mode, the project outputs debugging data as
strings on RX/TX. When built in Release mode, it outputs a binary
protocol format on RX/TX and is intended for those pins to be
wired to an XBee.