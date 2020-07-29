/*
  
*/

#include <arduino-timer.h>

#define LED_0_PIN 2
#define LED_1_PIN 3

#define SEL_SPRNKLR_BTN A3
#define ADD_TIME_BTN    A4

#define SET_PERIOD 20000         // Allow 20 sec for setting runtime

int minsLeft = 0;
bool setState = false;
bool runState = false;
unsigned long runMillis = 0;

// --------------------------------------
// Timers
//
auto timer = timer_create_default(); // create a timer with default settings

// 25ms Ticks of LED state
#define TICKS_MIN_H 6
#define TICKS_MIN_L 12
#define TICKS_PAUSE 36

int state_mins = 0;
int state_ticks = 0;
int state_mins_ticks;
const int num_mins_ticks = TICKS_MIN_H + TICKS_MIN_L;

bool blink_x_times(void *) {

  if (setState==true && runState==false && millis()>runMillis) {
    setState = false;
    runState = true;
  }
  if (runState == false) return true;

  if (state_ticks == 0) {
    state_ticks = minsLeft*(num_mins_ticks) + TICKS_PAUSE;
    state_mins = minsLeft;
    if (minsLeft > 0) state_mins_ticks = num_mins_ticks;
  }

  if (state_ticks > TICKS_PAUSE) {
    if (state_mins > 0) {
      if (state_mins_ticks == num_mins_ticks)   digitalWrite(LED_BUILTIN, HIGH);
      else if (state_mins_ticks == TICKS_MIN_L) digitalWrite(LED_BUILTIN, LOW);
    }

    if (state_mins_ticks == 0) {
      if (state_mins > 0) state_mins--;
      state_mins_ticks = num_mins_ticks;
    } else state_mins_ticks--;
  }

  if (state_ticks > 0) state_ticks--;
  return true;
}

bool changeMins(void *) {
  if (minsLeft > 0 && millis() > (runMillis + 10000) ) minsLeft--;
  return true;
}

// --------------------------------------
// Buttons
//
size_t b_state_0[2];
size_t b_state_1[2];
size_t b_state_2[2];

bool selState = false;
bool selReleased = false;
bool addState = false;
bool addReleased = false;

bool check_button(void *button) {

  size_t but = (size_t) button;
  int idx;
  if (but == (size_t) SEL_SPRNKLR_BTN) idx = 0;
  else idx = 1;

  b_state_0[idx] = b_state_1[idx];
  b_state_1[idx] = b_state_2[idx];
  b_state_2[idx] = digitalRead(but);

  if (b_state_0[idx]==LOW && b_state_1[idx]==LOW && b_state_1[idx]==LOW) {
    if (idx == 0) {
      if (selState==false) selState = true;
    } else {
      if (addState==false) addState = true;
    }
  }

  if (b_state_0[idx]==HIGH && b_state_1[idx]==HIGH && b_state_1[idx]==HIGH) {
    if (idx == 0) {
      if (selState==true) selReleased = true;
    } else {
      if (addState==true) addReleased = true;
    }
  }

  return true;
}

bool prevSelState = false;
bool handle_selButton(void *) {

  // Handle condition
  if (selState==true && prevSelState==false && selReleased==false) {
    if (runState==false) {
      digitalWrite(LED_0_PIN, !digitalRead(LED_0_PIN)); // toggle LED 0
      digitalWrite(LED_1_PIN, !digitalRead(LED_0_PIN)); // set to opposite of the other
    }
  }

  // Reset condition
  if (selState==true && selReleased==true) {
    selState = false;
    selReleased = false;
  }

  prevSelState = selState;
  return true;
}

bool prevAddState = false;
bool handle_addButton(void *) {

  // Handle condition
  if (addState==true && prevAddState==false && addReleased==false) {
    if (setState == false && runState == false) {
      setState = true;
      runMillis = millis() + SET_PERIOD;
    }

    if ( setState == true && minsLeft < 10 ) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(75);
      digitalWrite(LED_BUILTIN, LOW);
      minsLeft++;
    }
  }

  // Reset condition
  if (addState==true && addReleased==true) {
    addState = false;
    addReleased = false;
  }

  prevAddState = addState;
  return true;
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_0_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_0_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(SEL_SPRNKLR_BTN, INPUT_PULLUP);
  pinMode(ADD_TIME_BTN, INPUT_PULLUP);

  timer.every(25, blink_x_times);
  timer.every(30000, changeMins);

  timer.every(20, check_button, (void *)SEL_SPRNKLR_BTN);
  timer.every(400, handle_selButton);

  timer.every(20, check_button, (void *)ADD_TIME_BTN);
  timer.every(400, handle_addButton);
}

void loop() {
  timer.tick(); // tick the timer
}
