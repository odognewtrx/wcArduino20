/*
  
*/

#include <arduino-timer.h>
#include <ezButton.h>

#define LED_0_PIN 2
#define LED_1_PIN 3

#define SEL_SPRNKLR_BTN A3
#define ADD_TIME_BTN    A4

int minsLeft = 0;

void toggle_sel_led() {
  digitalWrite(LED_0_PIN, !digitalRead(LED_0_PIN)); // toggle LED 0
  digitalWrite(LED_1_PIN, !digitalRead(LED_0_PIN)); // set to opposite of the other
}

// --------------------------------------
// Buttons
//
ezButton selBtn(SEL_SPRNKLR_BTN);
ezButton addBtn(ADD_TIME_BTN);

unsigned long selPressTime  = 0;
unsigned long selRelTime = 1;
unsigned long addPressTime  = 0;
unsigned long addRelTime = 1;
int selCount = 0;
int addCount = 0;

void loop_buttons() {

  if (selBtn.isPressed()) {
    Serial.println("sel Button pressed.");
    selCount++;
    selPressTime = millis();
  }
  if (addBtn.isPressed()) {
    Serial.println("add Button pressed.");
    addCount++;
    addPressTime = millis();
  }

  if (selBtn.isReleased()) {
    selCount = 0;
    selRelTime = millis();
  }
  if (addBtn.isReleased()) {
    addCount = 0;
    addRelTime = millis();
  }

  
  if      ((selPressTime > selRelTime) &&
           (addPressTime < addRelTime) &&
           (selCount > 0) &&
           (selBtn.getStateRaw() == 0)) {
            toggle_sel_led();
            selCount++;
           }
  else if ((selPressTime < selRelTime) &&
           (addPressTime > addRelTime) &&
           (addCount > 0) &&
           (addBtn.getStateRaw() == 0)) {
            if (minsLeft < 10) minsLeft++;
            addCount++;
           }
}

// --------------------------------------
// Timers
//
auto timer = timer_create_default(); // create a timer with default settings

//bool toggle_led(void *) {
bool toggle_led() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // keep timer active? true
}


bool blink_x_times(void *) {
  int num = minsLeft;

  if (1) {
    Serial.print("blink_x_times: ");
    Serial.println(num);
  }

  for (int i=0; i<num; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    if (i < num-1) delay(275);
  }

  delay(2000);
  return true;
}

bool changeMins(void *) {
  if (minsLeft < 10) minsLeft++;
  else minsLeft = 0;
}

bool loopPeriodicFuncs(void *) {

  selBtn.loop();
  addBtn.loop();
  loop_buttons();
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  selBtn.setDebounceTime(50);
  addBtn.setDebounceTime(50);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_0_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_0_PIN, HIGH);

  //timer.every(100, loopPeriodicFuncs);

  // call the repeat_x_times function every 500 millis
  timer.every(500, blink_x_times);

  timer.every(5000, changeMins);
}

unsigned long loopMillis = 0;
void loop() {

  if (millis() > loopMillis + 100 ) {
    //Serial.println("Running Buttons");
    selBtn.loop();
    addBtn.loop();
    //Serial.print("sel Button: ");
    //Serial.println(selBtn.getStateRaw());
    if (selBtn.isPressed()) {
      Serial.println("sel Button pressed.");
    }
    loopMillis = millis();
  }
  timer.tick(); // tick the timer
}
