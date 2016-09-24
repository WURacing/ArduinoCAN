/*
 created 24 Sep 2016
 by Evan Simkowitz
 
 modified from SD Logger Tutorial by Sparkfun Electronics
 
 Modified by Toni Klopfenstein @ SparkFun Electronics
	September 2015
	https://github.com/sparkfun/CAN-Bus_Shield
 
 SD Card Datalogger
 
 This example is based off an example code from Arduino's site
 http://arduino.cc/en/Tutorial/Datalogger and it shows how to 
 log data from three analog sensors with a timestamp based on when
 the Arduino began running the current program to an SD card using
 the SD library https://github.com/greiman/SdFat by William 
 Greiman. This example code also includes an output to the 
 Serial Monitor for debugging.
 	
 The circuit:
 * analog sensors on analog pins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 This example code is in the public domain.
 */
 
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

//define the I2C slave address for sensor 1
#define I2C_SLAVE1_ADDR 0x04


 // Create the data structure to store the union between a 4-byte array and a float
union {
    	char bVal[4];
    	float fVal;
    } volatile wheelSpeed;


// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.

// Chip Select pin is tied to pin 9 on the SparkFun CAN-Bus Shield
const int chipSelect = 9;  
#define FILTER_COUNTS 7
float last_vals[FILTER_COUNTS];
int counter = 0;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  //initialize wheelSpeed float value
  wheelSpeed.fVal = 0.0;
}

void loop()
{
  // make a string for assembling the data to log:
  String dataString = "";

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  // this opens the file and appends to the end of file
  // if the file does not exist, this will create a new file.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile)   {  
    int timeStamp = millis();
    //write to uSD card
    dataFile.print(timeStamp);
    dataFile.print(" ms");
    dataFile.print(", ");
    //output also on Serial monitor for debugging
    Serial.print(timeStamp);
    Serial.print(",");

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
    for (auto val : last_vals) {
    	avg += val;
    }
    avg = avg/(float)FILTER_COUNTS;
    Serial.println(avg);
    dataFile.println(wheelSpeed.fVal);
    /*
    // read three sensors on A0, A1, and A2 while appending to the string:
    for (int analogPin = 0; analogPin < 3; analogPin++) 
    {
      int sensorVal = analogRead(analogPin);
      //write analog sensor data to uSD card
      dataFile.print(" Analog Pin A");
      dataFile.print(analogPin);
      dataFile.print(" = ");
      dataFile.print(sensorVal);
      //output also on Serial monitor for debugging
      Serial.print(" Analog Pin A");
      Serial.print(analogPin);
      Serial.print(" = ");
      Serial.print(sensorVal);
      //place comma between the analog sensor data
      if (analogPin < 3) 
      {
        dataString += ","; 
      }
    }
    */
    dataFile.println(); //create a new row to read data more clearly
    dataFile.close();   //close file
    Serial.println();   //print to the serial port too:

  }  
  // if the file isn't open, pop up an error:
  else
  {
    Serial.println("error opening datalog.txt");

  } 
}
