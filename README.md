# ArduinoCAN
WURacing Electronics Team Spring 2016

The purpose of this code is to read packets coming from our AEM Infinity ECU over the
AEMNet protocol and write it over XBee to a computer. 

Hardware-wise, we are using an Arduino UNO, the SparkFun CAN-Bus Shield, a pair of XBee radios, 
and a custom-made dashboard. The dashboard contains a bunch of LEDs for our RPM display, an
OLED for whatever you want (we are using it to display the numeric RPM), and a check engine
light. We are also using a custom connector that takes the four wires from
the AEMNet connector and maps them to a DB9 connector to connect to our CAN-Bus Shield.

Be aware that to read the XBee stream, we wrote our own reader on the computer side which
displays our data in real-time and writes it to a CSV. Because our ECU writes data over in 
packets, we left most of the sorting to our Python application, which reads the data in 
and organizes it before saving it.

We have updated the code to store a backup of the data to an SD card in case the stream is 
disrupted. This requires no extra hardware because the SparkFun CAN-Bus Shield has a built-in 
Micro-SD card reader.

Many of the libraries have been adapted to work with AEMNet, which varies slightly from the
CAN-Bus standard.
