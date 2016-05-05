/*
ArduinoCan.ino

WURacing Electronics Team Spring 2016
Carter Durno, Michael Greer, Thomas Kelly, Evan Simkowitz

The purpose of this code is to read packets coming from our AEM Infinity ECU over the
AEMNet protocol and write it over XBee to a computer.

Be aware that to read the XBee stream, we wrote our own reader on the computer side which
displays our data in real-time and writes it to a CSV. Because our ECU writes data over in
packets, we left most of the sorting to our Python application, which reads the data in
and organizes it before saving it.

Many of the libraries have been adapted to work with AEMNet, which varies slightly from the
CAN-Bus standard.

UPDATE 5/3/16: After burning out our second large OLED display, we are now using a smaller one
we found in the garage. Because of its small size, we have decided to only display one value at
a time and shift through them using a button. This change is reflected in the code below, though
we chose to leave the OLED display code commented out in case we decide to use a larger display
later. Be careful, though, if you decide to uncomment it, that you don't delete parts of the current
display code, as we were able to reuse some of it. You may have to play around with it to get it
working for the large display.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1305.h>
#include <Adafruit_SSD1306.h>

// Used for software SPI
//#define OLED_CLK 13
//#define OLED_MOSI 11

// Used for software or hardware SPI
//#define OLED_CS A5
//#define OLED_DC 8

// Used for I2C or SPI
#define OLED_RESET 4


// software SPI
Adafruit_SSD1306 display(OLED_RESET);
// hardware SPI
//Adafruit_SSD1305 display(OLED_DC, OLED_RESET, OLED_CS);

#include "Canbus.h"  // don't forget to include these
#include "defaults.h"
#include "global.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"
#include "stdlib.h"

#include <math.h>

#include <SoftwareSerial.h>
//#include "PacketSender.h"


#include <SPI.h>
#include <Wire.h>

#define LOGO16_GLCD_HEIGHT 64
#define LOGO16_GLCD_WIDTH  128
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};


/* MESSAGE IDS:

  MESSAGE_ONE:
  bytes 0 and 1 --> engine speed --> scale by .39063 rpm/bit
  bytes 2 and 3 --> engine load --> scale by .0015259 % / bit
  bytes 4 and 5 --> throttle --> scale by .0015259 % / bit
  byte 6 --> air temp --> no scale, 1 bit = 1 deg c
  byte 7 --> coolant temp --> no scale, 1 bit = 1 deg c

  MESSAGE_TWO:
  analog sensors --> 2 bytes for each sensor --> each scaled by .00007782 V/bit

  MESSAGE_THREE:
  same as message two

  MESSAGE_FOUR:
  byte 0 --> O2 sensor 1 --> .00390625 lambda / bit
  byte 1 --> O2 sensor 2 --> .00390625 lambda / bit
  bytes 2 and 3 --> vehicle speed --> .00390625 mph/ bit
  byte 4 --> gear calculated --> no scaling
  byte 5 --> ign timing --> .35156 deg/bit
  bytes 6 and 7 --> battery voltage --> .0002455 V/bit


*/
const int MESSAGE_ONE = 4294942720;
const int MESSAGE_TWO = 4294942721;
const int MESSAGE_THREE = 4294942722;
const int MESSAGE_FOUR = 4294942723;

const float RPM_SCALE = .39063;
const float ENG_LOAD_SCALE = .0015259;
const float ENG_THROTTLE_SCALE = .0015259;
const float ANALOG_SCALE = .00007782;
const float O2_SCALE = .00390625;
const float SPEED_SCALE = .00390625;
const float IGN_SCALE = .35156;
const float BATT_VOLTAGE_SCALE = .0002455;

const int displayDeltaTime = 250;
unsigned long displayLoopEndTime = 6000;

const int rpmDeltaTime = 50;
unsigned long rpmLoopEndTime = 3500;

unsigned long loopEndTime = 1000;

int8_t coolantC;

double rpm;
double load;
double coolantF;
double o2;
double vehicleSpeed;
byte gear;
double volts;

#define RPM_INDEX 0
#define LOAD_INDEX 1
#define COOLANT_INDEX 2
#define O2_INDEX 3
#define SPEED_INDEX 4
#define GEAR_INDEX 5
#define VOLTS_INDEX 6

#define NUM_OUTPUTS 7

double packet[7];

boolean warning = false;
boolean showedWarning = false;

boolean noData = false;

int noDataThreshold = 5000;
unsigned long noDataTime = noDataThreshold;

#define OVERHEATING 93
#define REDLINE 12000


#define rxPinXBee 8
#define txPinXBee 9

#define RPM_MAX 12000
#define RPM_MIN 0


#define buttonPin 6
unsigned long int buttonReaderAccumulator = 0;
const unsigned int buttonReaderDelay = 20;
bool buttons[3];
unsigned int counter = 0;
enum ButtonMode {RPM, TEMP, GEAR, NODATA};
ButtonMode buttonMode = RPM;
ButtonMode cachedButtonMode = RPM;
ButtonMode newButtonMode = RPM;

SoftwareSerial XBee(rxPinXBee, txPinXBee);
//PacketSender toRadio(XBee);

int SER_Pin = A0;   //pin 14 on the 75HC595
int RCLK_Pin = A1;  //pin 12 on the 75HC595
int SRCLK_Pin = A2; //pin 11 on the 75HC595

//How many of the shift registers - change this
#define number_of_74hc595s 3

//do not touch
#define numOfRegisterPins number_of_74hc595s * 8

boolean registers[numOfRegisterPins];

void setup() {

  XBee.begin(9600);

  rpm = 0;
  load = 0;
  coolantC = 0;
  coolantF = 0;
  vehicleSpeed = 0;
  gear = 0;
  volts = 0;
  o2 = 0;

  pinMode(SER_Pin, OUTPUT);
  pinMode(RCLK_Pin, OUTPUT);
  pinMode(SRCLK_Pin, OUTPUT);

  pinMode(buttonPin, INPUT);

  //reset all register pins
  clearRegisters();
  writeRegisters();

  Serial.begin(9600);
  //Initialise MCP2515 CAN controller at the specified speed
  if (Canbus.init(CANSPEED_500)) {
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Couldn't Init CAN");
  }

  delay(500);

  display.begin();
  display.setRotation(2);
  display.display();
  delay(3000);

}

void loop() {

  switch (buttonMode) {
  case RPM:
    {
      newButtonMode = TEMP;
      break;
    }
  case TEMP:
    {
      newButtonMode = GEAR;
      break;
    }
  case GEAR:
    {
      newButtonMode = RPM;
      break;
    }
  default:
    {
      newButtonMode = RPM;
      break;
    }
  }
  
  /*
   * This part of the code is where we set the display, the LED array,
   * and where we call the method that streams our data to the XBee.
   */
  if (millis() > rpmLoopEndTime){
    setLights();
    rpmLoopEndTime += rpmDeltaTime;
  }
  
  if (millis() > displayLoopEndTime) {
    boolean sendToXbee = true;
    
    display.clearDisplay();

    display.setTextSize(3);
    display.setTextColor(WHITE);

    if (coolantC >= OVERHEATING || rpm >= REDLINE) {
    showedWarning =  !(showedWarning);
      if (showedWarning){
          warning = !(warning);
      }
    }
    else if (warning) {
    warning = false;
    showedWarning = false;
    }

    ButtonMode tempButtonMode;

    if (coolantC >= OVERHEATING){
        tempButtonMode = TEMP;
    }
    else if (rpm >= REDLINE){
        tempButtonMode = RPM;
    }
    else{
      tempButtonMode = buttonMode;
    }

    /*
     * This switch statement writes different data to the display depending on
     * what mode we are in. The enumerable buttonMode can be rpm, temp, or gear.
     * In each case, the corresponding value is written to the display and the next
     * mode is stored to the enumerable newButtonMode. Because we only want to change
     * modes when the button is pressed, ButtonMode is not set to newButtonMode until
     * later in the code when the button detection is occurring.
     */
     if (warning){
      display.setCursor(0, 0);
      display.println("WARNING");
     }
     else{
      switch (tempButtonMode) {
        case RPM:
          {
            display.setCursor(0, 0);
            display.print((int)rpm);
            display.setTextSize(2);
            display.println("rpm");
            break;
          }
        case TEMP:
          {
            display.setCursor(0, 0);
            display.print(coolantF,1);
            display.setTextSize(2);
            display.setCursor(115, 0);
            display.print('F');
            display.drawCircle(110, 2, 2, WHITE);
            break;
          }
        case GEAR:
          {
            display.setTextSize(2);
            display.setCursor(0, 0);
            display.print("Gear: ");
            display.setTextSize(3);
            display.print(gear);
            break;
          }
        case NODATA:
          {
            display.setCursor(0,0);
            display.setTextSize(3);
            display.println("NO DATA");
            sendToXbee = false;
            break;
          }
        default:
          {
            display.setCursor(0, 0);
            display.print((int)rpm);
            display.setTextSize(2);
            display.println("rpm");
            break;
          }
      }
     }
    
    /*display.setCursor(0, 20);
      display.println("Gear:");
      display.setCursor(0, 28);
      display.setTextSize(2);
      display.println(gear);


      display.setCursor(115, 28);
      display.print('F');

      display.drawCircle(110, 28, 2, WHITE);

      if (coolantC >= OVERHEATING || rpm >= REDLINE) {
      warning =  !(warning);
      }
      else if (warning) {
      warning = false;
      }

      if (warning) {
      display.setCursor(22, 50);
      display.setTextSize(2);
      display.print("WARNING");
      }

      int digits = (int) (log10(abs(coolantF)));

      display.setTextSize(2);
      display.setCursor(72 - 12 * digits, 28);
      display.print(coolantF, 1);*/

    display.display();
    
    if(sendToXbee){
      writePacket();
    }

    displayLoopEndTime += displayDeltaTime;
  }

  /*
   * This portion of the code is where the CAN messages are actually
   * dissected and where we update our data values.
   */
  tCAN message;

  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {

      Serial.println("We found data");
      noData = false;
      if (buttonMode == NODATA){
        buttonMode = cachedButtonMode;
      }

      switch (message.id) {

        case MESSAGE_ONE: {
            uint16_t rawRPM = (uint16_t)message.data[0] << 8;
            rawRPM |= message.data[1];
            rpm = rawRPM * RPM_SCALE;

            packet[RPM_INDEX] = rpm;
            
            uint16_t rawLoad = (uint16_t)message.data[2] << 8;
            rawLoad |= message.data[3];
            load = rawLoad * ENG_LOAD_SCALE;

            packet[LOAD_INDEX] = load;

            coolantC = message.data[7];

            coolantF = ((double)coolantC * 1.8) + 32;

            packet[COOLANT_INDEX] = coolantF;

            break;
          }

        case MESSAGE_FOUR: {
            uint8_t rawo2 = (uint8_t)message.data[0];
            o2 = rawo2 * O2_SCALE + 0.5;

            packet[O2_INDEX] = o2;

            uint16_t rawSpeed = (uint16_t)message.data[2] << 8;
            rawSpeed |= message.data[3];
            vehicleSpeed = rawSpeed * SPEED_SCALE;

            packet[SPEED_INDEX] = vehicleSpeed;

            gear = message.data[4];

            packet[GEAR_INDEX] = (double)gear;

            uint16_t rawVolts = (uint16_t)message.data[7] << 8;
            rawVolts |= message.data[8];
            volts = rawVolts * BATT_VOLTAGE_SCALE;

            packet[VOLTS_INDEX] = volts;
            break;
          }

        default: {
            break;
          }

      }
    }
  }
  else {
    Serial.println("No data");
    if (noData){
      if (millis() >=  noDataTime + noDataThreshold){
        chachedButtonMode = buttonMode;
        buttonMode = NODATA;
      }
    }
    else{
      noData = true;
      noDataTime = millis();
    }
  }

  /*
   * All this does is loop our values without requiring us to hook up
   * to the ECU.
   */
  /*if (millis() > loopEndTime) {
    ++coolantC;
    ++gear;
    rpm += 500;
    loopEndTime += 1000;

    coolantF = ((double)coolantC * 1.8) + 32;

    packet[COOLANT_INDEX] = coolantF;
    packet[GEAR_INDEX] = gear;
    packet[RPM_INDEX] = rpm;

    if (rpm > RPM_MAX + 1000) {
      rpm -= RPM_MAX;
      gear = 0;
      coolantC = 0;
    }
  }*/

  /* 
   *  This is a simple debouncing filter for our mode-switching button. The
   *  way it's set up, it will debounce a button press and then set the mode
   *  equal to the new mode set in the switch statement. If the value of buttons[2]
   *  is different from that of buttons[1], it will add a delay to prevent unnecessary
   *  readings of the button and to make the code run more efficiently. The way the
   *  filter works, it will only register a button press when the button is depressed.
   *  This makes the reading more accurate and helps compensate for lower-quality 
   *  button contacts.
   */
  bool buttonPressed = false;
  readButton();
  if (buttons[2] != buttons[1]) {
    buttonReaderAccumulator = millis();
  }
  // actual debouncing filter
  if (millis() - buttonReaderAccumulator > buttonReaderDelay) {
    if (buttons[2] != buttons[0]) {
      buttons[0] = buttons[2];
      if (!buttons[0]) {
        buttonPressed = true;
      }
    }
  }
  if (buttonPressed) {
    buttonMode = newButtonMode;
    String str = "new mode: ";
    str += buttonMode;
    Serial.println(str);
  }
  buttons[1] = buttons[2];
}

/*
 * Reads the current value of the button at buttonPin as a boolean value
 * and sets the third value of the boolean array buttons[] equal to the value
 * of buttonPin.
 */
void readButton() {
  buttons[2] = digitalRead(buttonPin);
}

/*
 * This method and the next couple control the shift registers that power
 * the LED array.
 */
void clearRegisters() {
  for (int i = numOfRegisterPins - 1; i >=  0; i--) {
    registers[i] = LOW;
  }
  writeRegisters();
}


//Set and display registers
//Only call AFTER all values are set how you would like (slow otherwise)
void writeRegisters() {

  digitalWrite(RCLK_Pin, LOW);

  for (int i = numOfRegisterPins - 1; i >=  0; i--) {
    digitalWrite(SRCLK_Pin, LOW);

    int val = registers[i];

    digitalWrite(SER_Pin, val);
    digitalWrite(SRCLK_Pin, HIGH);

  }
  digitalWrite(RCLK_Pin, HIGH);

}

//set an individual pin HIGH or LOW
void setRegisterPin(int index, int value) {
  registers[index] = value;
}

//Set the lights according to the RPM value
void setLights() {

  clearRegisters();

  int temp = map(rpm, 0, RPM_MAX, 0, 10);

  for (int i = 0; i < temp; i++) {
    setRegisterPin(i, HIGH);
    setRegisterPin(19 - i, HIGH);
    writeRegisters();
  }
}

/*
 * This method writes data to our XBee to be relayed back to the computer in the
 * pit. To use this code, you must be using our accompanying computer application.
 */
void writePacket() {
  Serial.println("writing stuff to xbee serial");
  for (int i = 0; i < NUM_OUTPUTS; ++i) {
    XBee.print(packet[i]);
    XBee.print(',');
  }
  XBee.println(millis());
}






