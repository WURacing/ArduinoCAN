// An example to get started with the TMP006 temperature sensor
//Modified by SparkFun Electronics.
//We release this code under ([Beerware license](http://en.wikipedia.org/wiki/Beerware)).

//Original Code license:
/***************************************************
  >  This is a library for the TMP006 Temp Sensor
  >
  >  Designed specifically to work with the Adafruit TMP006 Breakout
  >  ----> https://www.adafruit.com/products/1296
  >
  >  These displays use I2C to communicate, 2 pins are required to
  >  interface
  >  Adafruit invests time and resources providing this open source code,
  >  please support Adafruit and open-source hardware by purchasing
  >  products from Adafruit!
  >
  >  Written by Limor Fried/Ladyada for Adafruit Industries.
  >  BSD license, all text above must be included in any redistribution
  > ****************************************************/

// Only things needed to configure are the sensor address and
// samples per reading, however the defaults work fine if using
// just one TMP006 sensor. Check out the hook up guide tutorial
// if you want a better explanation of the hardware and code.

#include <stdint.h>
#include <math.h>
#include <Wire.h>
#include "I2C_16.h"
#include "TMP006.h"

<<<<<<< Updated upstream
uint8_t sensorArray[] = {0x40, 0x41, 0x42, 0x43}; // Array of I2C addresses for TMP006 sensors (addresses can be 0x40-0x47)
uint16_t samples = TMP006_CFG_16SAMPLE; // # of samples per reading, can be 1/2/4/8/16
=======
uint8_t addrs[1] = {0x40}; //, 0x41, 0x42, 0x43} ; // I2C address of TMP006, can be 0x40-0x47
uint16_t samples = TMP006_CFG_8SAMPLE; // # of samples per reading, can be 1/2/4/8/16
>>>>>>> Stashed changes

void setup()
{
  float objectTemps[4];
  Serial.begin(9600);
  Serial.println("TMP006 Example");

  for (auto addr : addrs) {

    config_TMP006(addr, samples);


  }
}

void loop()
{

  
  for (auto addr : addrs) {
    float object_temp = readObjTempC(addr);
    Serial.print("Object Temperature: ");
    Serial.print(object_temp); Serial.println("*C");
    Serial.println(addr, HEX);


  }

  delay(2000); // delay 1 second for every 4 samples per reading
}
