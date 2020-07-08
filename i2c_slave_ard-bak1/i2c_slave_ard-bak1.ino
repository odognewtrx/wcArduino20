

// Include the Wire library for I2C
#include <Wire.h>
#include <EEPROM.h>

// LED on pin 13
const int ledPin = 13; 

struct Limit {
  int valid;   // when == 1234
  int lim;
};
struct Limits {
  int valid;   // when == 1234
  int minLim;
  int maxLim;
};

const int LIM_VALID = 1234;

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor

Limits getLimits() {

  Limits curLimits;
  Limit gLim;
  curLimits.valid = 0;
  
  EEPROM.get(0, gLim);   // min limit
  if (gLim.valid == 1234) {
    curLimits.valid++;
    curLimits.minLim = gLim.lim;
  }
  
  EEPROM.get(sizeof(Limit), gLim);   // max limit
  if (gLim.valid == 1234) {
    curLimits.valid++;
    curLimits.maxLim = gLim.lim;
  }

  if (curLimits.valid != 2) {
    curLimits.valid = 0;
    //curLimits.minLim = 500;
    //curLimits.maxLim = 520;
  }
  return curLimits;
}

void setLimit(int maxNotMin, int lim) { // min: maxNotMin==0 max: maxNotMin==1
  Limit sLim;
  sLim.valid = 1234;
  sLim.lim = lim;
  EEPROM.put(maxNotMin*sizeof(Limit), sLim);
}

void setup() {
  // Join I2C bus as slave with address 8
  Wire.begin(0x8);
  Serial.begin(9600);  // start serial for output

  Limits curLimits = getLimits();
  Serial.println(curLimits.valid);
  Serial.println(curLimits.minLim);
  Serial.println(curLimits.maxLim);

  if (curLimits.valid < 1) {
    Serial.println("Setting Limits...");
    setLimit(0, 321);
    setLimit(1, 876);
  }
  
  // Call receiveEvent when data received                
  Wire.onReceive(receiveEvent);

  Wire.onRequest(requestEvent); // register event
  
  // Setup pin 13 as output and turn LED off
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

// Function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (Wire.available()) { // loop through all but the last
    char c = Wire.read();    // receive byte as a character
    digitalWrite(ledPin, c);
  }
}

int reqState = 0;
uint8_t sense;

void requestEvent() {
  
  if (reqState == 0) {
    reqState = 1;
    sense = sensorValue>>8;
  } else {
    sense = sensorValue&0xFF;
    reqState = 0;
  }
  Wire.write(&sense, 1);
}

void loop() {
  delay(100);
  if (reqState == 0) {
    sensorValue = analogRead(sensorPin);
  }
}
