#include "Arduino.h"
#include <ezButton.h>

// -----------------------------------------------------------
// Button management
//
#define ADDMIN_BTN_PIN    2
#define CANCEL_BTN_PIN    3
#define SEL_SPRINKLER_PIN 4
#define ENABLE_0_PIN      5
#define ENABLE_1_PIN      6
#define SHORT_PRESS_TIME  250  //250 milliseconds

ezButton addMinBtn(ADDMIN_BTN_PIN);
ezButton cancelBtn(CANCEL_BTN_PIN);

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
unsigned long startTime = 0;
unsigned long stopTime = 0;

void addOneMin() {
  if (startTime == 0 ) {
    startTime = millis();
    stopTime = startTime;
  }
  stopTime += 60000;
}

void cancelRun() {
  startTime = 0;
  stopTime = 0;
}

void button_loop() {

  if(addMinBtn.isPressed())
    pressedTime = millis();

  if(cancelBtn.isPressed())
    pressedTime = millis();

  if(addMinBtn.isReleased()) {

    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME ) {
      Serial.println("Adding 1 Minute...");
      addOneMin();
    }
  }

  if(cancelBtn.isReleased()) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME ) {
      Serial.println("Cancelling sprinkler...");
      cancelRun();
    }
  }
}

// -----------------------------------------------------------------------------
// SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(ENABLE_0_PIN, OUTPUT);
  digitalWrite(ENABLE_0_PIN, LOW);
  pinMode(ENABLE_1_PIN, OUTPUT);
  digitalWrite(ENABLE_1_PIN, LOW);

  addMinBtn.setDebounceTime(50);
  cancelBtn.setDebounceTime(50);
}

// -----------------------------------------------------------------------------
// LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP 
// -----------------------------------------------------------------------------
void loop() {
  addMinBtn.loop();
  cancelBtn.loop();

  button_loop();        // Check for button operations

  if (stopTime > startTime) {
    if (digitalRead(SEL_SPRINKLER_PIN)) {
      digitalWrite(ENABLE_1_PIN, HIGH);
    } else {
      digitalWrite(ENABLE_0_PIN, HIGH);
    }
  } else {
    digitalWrite(ENABLE_0_PIN, LOW);
    digitalWrite(ENABLE_1_PIN, LOW);
  }

  delay(100);
}
