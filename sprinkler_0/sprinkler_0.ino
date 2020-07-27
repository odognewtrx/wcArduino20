#include "Arduino.h"
#include <ezButton.h>

// -----------------------------------------------------------
// Button management
//
#define ADDMIN_BTN_PIN    2
#define CANCEL_BTN_PIN    3
#define SEL_SPRINKLER_PIN 4
#define ENABLE_1_PIN      5
#define ENABLE_2_PIN      6
#define SHORT_PRESS_TIME  250  //250 milliseconds

ezButton addMinBtn(ADDMIN_BTN_PIN);
ezButton cancelBtn(CANCEL_BTN_PIN);

unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

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
    }
  }

  if(cancelBtn.isReleased()) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME ) {
      Serial.println("Cancelling sprinkler...");
    }
  }

// -----------------------------------------------------------------------------
// SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(ENABLE_1_PIN, OUTPUT);
  digitalWrite(ENABLE_1_PIN, LOW);
  pinMode(ENABLE_2_PIN, OUTPUT);
  digitalWrite(ENABLE_2_PIN, LOW);

  addMinBtn.setDebounceTime(50);
  cancelBtn.setDebounceTime(50);
}

void loop() {
  addMinBtn.loop();
  cancelBtn.loop();

  button_loop();        // Check for button operations

  delay(200);
}
