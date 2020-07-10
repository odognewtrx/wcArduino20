#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>
#include <ezButton.h>

#define DIR_OPEN     0         // 
#define DIR_CLOSE    1
#define N_ENABLE_PIN 4
#define DIR_PIN      6
#define MOVE_PIN     5
#define NO_CHECK     1      // flag for button presses to bypass position check
#define CMD_OPEN     55
#define CMD_CLOSE    66

#define sensorPin    A0     // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor

//-----------------------------------------------
// Radio setup
//
// This is just the way the RF24 library works:
// Hardware configuration: Set up nRF24L01 radio on SPI bus (pins 11, 12, 13) plus pins CE & CSN
RF24 radio(9, 10);
byte addresses[][6] = {"1Node","2Node"};

//-----------------------------------------------
// Movement limit functions
//
#define LIM_VALID 1234    //prevent uninitialied EEPROM from reading as valid
struct Limit { // Struct for read/write to EEPROM
  int valid;   // when == LIM_VALID
  int lim;
};
struct Limits { // Struct to store limit read values
  int valid;   // when > 0
  int minLim;
  int maxLim;
};

Limits curLimits;    // store limits read from EEPROM

Limits getLimits() {
  Limit gLim;
  curLimits.valid = 0;
  
  EEPROM.get(0, gLim);   // min limit
  if (gLim.valid == LIM_VALID) {
    curLimits.valid++;
    curLimits.minLim = gLim.lim;
  }
  
  EEPROM.get(sizeof(Limit), gLim);   // max limit
  if (gLim.valid == LIM_VALID) {
    curLimits.valid++;
    curLimits.maxLim = gLim.lim;
  }

  if (curLimits.valid != 2) curLimits.valid = 0;
  Serial.println("");
  Serial.println("Limits:");
  Serial.println(curLimits.valid);
  Serial.println(curLimits.minLim);
  Serial.println(curLimits.maxLim);
}

void setLimit(int dir, int lim) {
  Limit sLim;
  int maxNotMin;
  if      (dir == DIR_OPEN) maxNotMin = 1;   // max limit: 2nd byte of EEPROM
  else if (dir == DIR_CLOSE) maxNotMin = 0;  // min limit: 1st byte of EEPROM
  sLim.valid = LIM_VALID;
  sLim.lim = lim;
  EEPROM.put(maxNotMin*sizeof(Limit), sLim);
  getLimits();
}

// -----------------------------------------------------------
// Stepper Movement
//
int moveStep(int direction, int noCheck=0) {
  if ( noCheck ||
       ((direction == DIR_OPEN) && (sensorValue < curLimits.maxLim)) ||
       ((direction == DIR_CLOSE) && (sensorValue > curLimits.minLim)) ) {

    digitalWrite(DIR_PIN, direction);
    digitalWrite(N_ENABLE_PIN, LOW);
    delay(10);
    for (int i=0; i<100; i++) {
      digitalWrite(MOVE_PIN, HIGH);
      delay(2);
      digitalWrite(MOVE_PIN, LOW);
      delay(2);
    }
    digitalWrite(N_ENABLE_PIN, HIGH);  
    delay(10);
  }
  return analogRead(sensorPin);
}

// -----------------------------------------------------------
// Button management for setting limits
//
#define OPEN_BTN_PIN    2
#define CLOSE_BTN_PIN   3
#define SHORT_PRESS_TIME  500   // 500 milliseconds
#define LONG_PRESS_TIME   2000  // 2000 milliseconds

ezButton openBtn(OPEN_BTN_PIN);
ezButton closeBtn(CLOSE_BTN_PIN);

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void button_loop() {
  if(openBtn.isPressed())
    pressedTime = millis();

  if(closeBtn.isPressed())
    pressedTime = millis();

  if(openBtn.isReleased()) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME )
      sensorValue = moveStep(DIR_OPEN, NO_CHECK);

    if( pressDuration > LONG_PRESS_TIME )
      setLimit(DIR_OPEN, sensorValue);
  }

  if(closeBtn.isReleased()) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME )
      sensorValue = moveStep(DIR_CLOSE, NO_CHECK);

    if( pressDuration > LONG_PRESS_TIME )
      setLimit(DIR_CLOSE, sensorValue);
  }
}
unsigned char prevCmd = 0;
int prevSensor = 2000;
long prevMillis = 0;
// -----------------------------------------------------------------------------
// SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(MOVE_PIN, OUTPUT);
  pinMode(N_ENABLE_PIN, OUTPUT);
  digitalWrite(N_ENABLE_PIN, HIGH);

  openBtn.setDebounceTime(50);
  closeBtn.setDebounceTime(50);

  getLimits();
  
  if (curLimits.valid < 1) {
    Serial.println("Setting Limits near middle...");
    setLimit(DIR_CLOSE, 500);
    setLimit(DIR_OPEN, 525);
  }
 
  Serial.println("THIS IS THE RECEIVER CODE - YOU NEED THE OTHER ARDUINO TO TRANSMIT");

  // Initiate the radio object
  radio.begin();

  // Set the transmit power to lowest available to prevent power supply related issues
  radio.setPALevel(RF24_PA_MIN);

  // Set the speed of the transmission to the quickest available
  radio.setDataRate(RF24_2MBPS);

  // Use a channel unlikely to be used by Wifi, Microwave ovens etc
  radio.setChannel(124);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1, addresses[1]);

  // Start the radio listening for data
  radio.startListening();
}

// -----------------------------------------------------------------------------
// We are LISTENING on this device only (although we do transmit a response)
// -----------------------------------------------------------------------------
void loop() {
  openBtn.loop();
  closeBtn.loop();

  sensorValue = analogRead(sensorPin);
  button_loop();        // Check for button operations

  unsigned char data;   // Store received data

  // Is there any data for us to get?
  if ( radio.available()) {

    // Go and read the data
    while (radio.available()) { radio.read( &data, sizeof(char)); }

    if ( (data != prevCmd) || ((millis() > prevMillis + 2000) && (
          (sensorValue < prevSensor - 5) ||
          (sensorValue > prevSensor + 5))) ) {
        //Serial.print("  prevSensor:  ");
        //Serial.print(prevSensor);
        //Serial.print("  prevMillis:  ");
        //Serial.print(prevMillis);
        //Serial.print("  Millis:  ");
        //Serial.print(millis());
        Serial.print("Got Command ");
        Serial.print(data);
        Serial.print("  sensor:  ");
        Serial.println(sensorValue);
        prevMillis = millis();
        prevSensor = sensorValue;
    }

    prevCmd = data;

    if (data == CMD_OPEN)  sensorValue = moveStep(DIR_OPEN);
    if (data == CMD_CLOSE) sensorValue = moveStep(DIR_CLOSE);

    // Send back sensor reading mapped into 8 bits
    radio.stopListening();
    data = map( constrain(sensorValue, curLimits.minLim, curLimits.maxLim),         //constrain
                                       curLimits.minLim, curLimits.maxLim, 0, 255); //map
    radio.write( &data, sizeof(char) );

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}
