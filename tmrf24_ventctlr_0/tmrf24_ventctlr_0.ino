#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include <ezButton.h>

// This is just the way the RF24 library works:
// Hardware configuration: Set up nRF24L01 radio on SPI bus (pins 11, 12, 13) plus pins CE & CSN:
RF24 radio(9, 10);  // CE and CSN

byte addresses[][6] = {"1Node", "2Node"};

// -----------------------------
//  I2C Interface
//
uint8_t cmdToRun = 11;
uint8_t sensorValue;

// ------------------------------
// LED Patterns and pin order
//
#define NUM_LEDS 7
uint8_t ledOrder[NUM_LEDS] =      {   2,    3,    4,    5,    6,    7,    8};
uint8_t positionPat_0[NUM_LEDS] = {HIGH,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW};
uint8_t positionPat_1[NUM_LEDS] = {HIGH, HIGH,  LOW,  LOW,  LOW,  LOW,  LOW};
uint8_t positionPat_2[NUM_LEDS] = {HIGH, HIGH, HIGH,  LOW,  LOW,  LOW,  LOW};
uint8_t positionPat_3[NUM_LEDS] = {HIGH, HIGH, HIGH, HIGH,  LOW,  LOW,  LOW};
uint8_t positionPat_4[NUM_LEDS] = {HIGH, HIGH, HIGH, HIGH, HIGH,  LOW,  LOW};
uint8_t positionPat_5[NUM_LEDS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,  LOW};
uint8_t positionPat_6[NUM_LEDS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
uint8_t errorPat_0[NUM_LEDS] =    {HIGH,  LOW, HIGH,  LOW, HIGH,  LOW, HIGH};
uint8_t errorPat_1[NUM_LEDS] =    { LOW, HIGH,  LOW, HIGH,  LOW, HIGH,  LOW};
uint8_t allOn[NUM_LEDS] =         {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

#define NUM_PATT 7
uint8_t sensorLims[NUM_PATT] =    {   1,   51,  102,  153,  204,  255,  255};
uint8_t * positionPat[NUM_PATT] = {positionPat_0, positionPat_1, positionPat_2, positionPat_3,
                                   positionPat_4, positionPat_5, positionPat_6};

void setLeds(uint8_t *patt) {for (int i=0; i<NUM_LEDS; i++) digitalWrite(ledOrder[i], patt[i]);}

void setPositionLeds(uint8_t val) {
  int j;
  for (j=0; j<NUM_PATT-1; j++) {
    if ( val < sensorLims[j] ) {
      setLeds( positionPat[j] );
      j = 100;   // exit function
    }
  }
  // fall through the for loop test
  if (j ==  NUM_PATT-1) setLeds( positionPat[j] );
}

// Animations (700ms delay)
void animOpen() {
  uint8_t patt[NUM_LEDS] = { LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW};
  for (int i=0; i<NUM_LEDS; i++) {
    if (i>0) patt[i-1] = LOW;
    patt[i] = HIGH;
    setLeds(patt);
    delay(100);
  }
}

void animClose() {
  uint8_t patt[NUM_LEDS] = { LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW};
  for (int i=NUM_LEDS-1; i>=0; i--) {
    if (i<NUM_LEDS-1) patt[i+1] = LOW;
    patt[i] = HIGH;
    setLeds(patt);
    delay(100);
  }
}

#define ST_LED_NORMAL 0
#define ST_LED_ERROR_0 1
#define ST_LED_ERROR_1 2
uint8_t ledState = ST_LED_NORMAL;

// --------------------------------------
// Buttons
//
ezButton openBtn(A1);
ezButton closeBtn(A2);

unsigned long openPressTime  = 0;
unsigned long openRelTime = 1;
unsigned long closePressTime  = 0;
unsigned long closeRelTime = 1;
int openCount = 0;
int closeCount = 0;

uint8_t loop_buttons() {
  uint8_t cmd = 11;

  if (openBtn.isPressed()) {
    Serial.println("Open Button pressed.");
    openCount++;
    openPressTime = millis();
  }
  if (closeBtn.isPressed()) {
    Serial.println("Close Button pressed.");
    closeCount++;
    closePressTime = millis();
  }

  if (openBtn.isReleased()) {
    openCount = 0;
    openRelTime = millis();
  }
  if (closeBtn.isReleased()) {
    closeCount = 0;
    closeRelTime = millis();
  }

  
  if      ((openPressTime > openRelTime) &&
           (closePressTime < closeRelTime) &&
           (openCount > 0) &&
           (openBtn.getStateRaw() == 0)) {
            cmd = 55;
            openCount++;
           }
  else if ((openPressTime < openRelTime) &&
           (closePressTime > closeRelTime) &&
           (closeCount > 0) &&
           (closeBtn.getStateRaw() == 0)) {
            cmd = 66;
            closeCount++;
           }
  //else if ((openPressTime > openRelTime) && (closePressTime > closeRelTime)) cmd = 77;
  
  return cmd;
}

// -----------------------------------------------------------------------------
// SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP   SETUP
// -----------------------------------------------------------------------------
void setup() {

  Serial.begin(9600);
  Serial.println("THIS IS THE TRANSMITTER CODE - YOU NEED THE OTHER ARDIUNO TO SEND BACK A RESPONSE");

  openBtn.setDebounceTime(50);
  closeBtn.setDebounceTime(50);

  // Initiate the radio object
  radio.begin();

  // Display LEDs
  for (int i=0; i<NUM_LEDS; i++) pinMode(ledOrder[i], OUTPUT);

  // Set the transmit power to lowest available to prevent power supply related issues
  //radio.setPALevel(RF24_PA_MIN);
  radio.setPALevel(RF24_PA_LOW);

  // Set the speed of the transmission to the quickest available
  //radio.setDataRate(RF24_2MBPS);
  radio.setDataRate(RF24_1MBPS);

  // Use a channel unlikely to be used by Wifi, Microwave ovens etc
  radio.setChannel(124);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);

  setLeds( allOn );
  delay(500);
}

// -----------------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// -----------------------------------------------------------------------------
void loop() {
  openBtn.loop();
  closeBtn.loop();

  // Check buttons for commands
  cmdToRun = loop_buttons();
  
  uint8_t data = cmdToRun;
    
  // Ensure we have stopped listening (even if we're not) or we won't be able to transmit
  radio.stopListening(); 

  // Did we manage to SUCCESSFULLY transmit that (by getting an acknowledgement back from the other Arduino)?
  // Even we didn't we'll continue with the sketch, you never know, the radio fairies may help us
  if (!radio.write( &data, sizeof(unsigned char) )) {
    Serial.println("No acknowledgement of transmission - receiving radio device connected?");
    if      (ledState == ST_LED_NORMAL)  ledState = ST_LED_ERROR_0;
    else if (ledState == ST_LED_ERROR_0) ledState = ST_LED_ERROR_1;
    else if (ledState == ST_LED_ERROR_1) ledState = ST_LED_ERROR_0;
  } else ledState = ST_LED_NORMAL;

  // Now listen for a response
  radio.startListening();

  // Anmations should take the same time as the movement
  if ((cmdToRun == 55) && (sensorValue < 255)) animOpen();
  if ((cmdToRun == 66) && (sensorValue > 0  )) animClose();
  
  // But we won't listen for long, 200 milliseconds is enough
  unsigned long started_waiting_at = millis();

  // Loop here until we get indication that some data is ready for us to read (or we time out)
  while ( ! radio.available() ) {

    // Oh dear, no response received within our timescale
    if (millis() - started_waiting_at > 200 ) {
      Serial.println("No response received - timeout!");
      if      (ledState == ST_LED_ERROR_0) setLeds( errorPat_0 );
      else if (ledState == ST_LED_ERROR_1) setLeds( errorPat_1 );
      return;
    }
  }

  // Now read the data that is waiting for us in the nRF24L01's buffer
  unsigned char dataRx;
  radio.read( &dataRx, sizeof(unsigned char) );

  // Show user what we sent and what we got back
  Serial.print("Sent: ");
  Serial.print(data);
  Serial.print(", received: ");
  Serial.println(dataRx);

  sensorValue = dataRx;
  cmdToRun = 11;

  if      (ledState == ST_LED_NORMAL)  setPositionLeds(sensorValue);
  else if (ledState == ST_LED_ERROR_0) setLeds( errorPat_0 );
  else if (ledState == ST_LED_ERROR_1) setLeds( errorPat_1 );
  
  // Try again .25s later
  delay(250);
}
