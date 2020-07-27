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

unsigned char prevCmd = 0;
int prevSensor = 2000;
long prevMillis = 0;
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

// -----------------------------------------------------------------------------
// We are LISTENING on this device only (although we do transmit a response)
// -----------------------------------------------------------------------------
void loop() {
  addMinBtn.loop();
  cancelBtn.loop();

  sensorValue = analogRead(sensorPin);
  button_loop();        // Check for button operations

  unsigned char data;   // Store received data

  // Is there any data for us to get?
  if ( radio.available()) {

    // Go and read the data
    while (radio.available()) { radio.read( &data, sizeof(char)); }

    //if ( millis() > prevMillis + 2000 ) {
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
        prevCmd = data;
    }

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
  delay(200);
}
