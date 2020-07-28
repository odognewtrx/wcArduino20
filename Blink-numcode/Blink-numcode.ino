/*
  
*/

#include <arduino-timer.h>

auto timer = timer_create_default(); // create a timer with default settings

//bool toggle_led(void *) {
bool toggle_led() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // keep timer active? true
}

int minsLeft = 0;

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

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // call the toggle_led function every 1000 millis (1 second)
  //timer.every(1000, toggle_led);

  // call the repeat_x_times function every 500 millis
  timer.every(500, blink_x_times);

  timer.every(5000, changeMins);
}

void loop() {
  timer.tick(); // tick the timer
}
