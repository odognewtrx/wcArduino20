

// Include the Wire library for I2C
#include <Wire.h>
#include <EEPROM.h>
#include <ezButton.h>

// LED on pin 13
const int ledPin = 13; 


const int LIM_VALID = 1234;
const int OPEN_BTN_PIN = 4;
const int CLOSE_BTN_PIN = 3;
const int DIR_OPEN = 1;         // 
const int DIR_CLOSE = 0;
const int N_ENABLE_PIN = 9;
const int DIR_PIN = 8;
const int MOVE_PIN = 7;
const int NO_CHECK = 1;   // flag for button presses to bypass position check

const int SHORT_PRESS_TIME = 500; // 500 milliseconds
const int LONG_PRESS_TIME  = 2000; // 2000 milliseconds

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor

ezButton openBtn(OPEN_BTN_PIN);
ezButton closeBtn(CLOSE_BTN_PIN);

struct Limit { // Struct for read/write to EEPROM
  int valid;   // when == 1234
  int lim;
};
struct Limits { // Struct to store limit read values
  int valid;   // when > 0
  int minLim;
  int maxLim;
};

Limits curLimits;

Limits getLimits() {

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

  if (curLimits.valid != 2) curLimits.valid = 0;
}

void setLimit(int maxNotMin, int lim) { // min: maxNotMin==0 max: maxNotMin==1
  Limit sLim;
  sLim.valid = 1234;
  sLim.lim = lim;
  EEPROM.put(maxNotMin*sizeof(Limit), sLim);
}

int moveStep(int direction, int noCheck=0) {
  if ( noCheck ||
       ((direction == DIR_OPEN) && (sensorValue < curLimits.maxLim)) ||
       ((direction == DIR_CLOSE) && (sensorValue > curLimits.minLim)) ) {

    digitalWrite(DIR_PIN, direction);
    digitalWrite(N_ENABLE_PIN, LOW);
    delay(10);
    digitalWrite(ledPin, HIGH);
    for (int i=0; i<100; i++) {
      digitalWrite(MOVE_PIN, HIGH);
      delay(2);
      digitalWrite(MOVE_PIN, LOW);
      delay(2);
    }
    digitalWrite(ledPin, LOW);
    digitalWrite(N_ENABLE_PIN, HIGH);
  
    delay(10);
  }
  return analogRead(sensorPin);
}

// Function that executes whenever data is received from master
void receiveEvent(int howMany) {
  while (Wire.available()) { // loop through all but the last
    uint8_t c = Wire.read();    // receive byte as a character
    if ( c == 1 ) sensorValue = moveStep(DIR_OPEN);
    if ( c == 0 ) sensorValue = moveStep(DIR_CLOSE);
  }
}

void requestEvent() {
  uint8_t reqDat = map( constrain(sensorValue, curLimits.minLim, curLimits.maxLim),
                        curLimits.minLim, curLimits.maxLim, 0, 255);
  Wire.write(&reqDat, 1);
}

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  // Join I2C bus as slave with address 8
  Wire.begin(0x8);
  Serial.begin(9600);  // start serial for output

  pinMode(DIR_PIN, OUTPUT);
  pinMode(MOVE_PIN, OUTPUT);
  pinMode(N_ENABLE_PIN, OUTPUT);
  digitalWrite(N_ENABLE_PIN, HIGH);

  openBtn.setDebounceTime(50);
  closeBtn.setDebounceTime(50);

  getLimits();
  Serial.println("");
  Serial.println("Limits:");
  Serial.println(curLimits.valid);
  Serial.println(curLimits.minLim);
  Serial.println(curLimits.maxLim);

  if (curLimits.valid < 1) {
    Serial.println("Setting Limits...");
    setLimit(0, 321);
    setLimit(1, 876);
    Serial.println(curLimits.valid);
    Serial.println(curLimits.minLim);
    Serial.println(curLimits.maxLim);
  }
  
  // Call receiveEvent when data received                
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent); // register event
  
  // Setup pin 13 as output and turn LED off
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}


void loop() {
  openBtn.loop();
  closeBtn.loop();

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
      Serial.println("Set Open Limit");
  }

  if(closeBtn.isReleased()) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME )
      sensorValue = moveStep(DIR_CLOSE, NO_CHECK);

    if( pressDuration > LONG_PRESS_TIME )
      Serial.println("Set Close Limit");
  }
  
  sensorValue = analogRead(sensorPin);
}
