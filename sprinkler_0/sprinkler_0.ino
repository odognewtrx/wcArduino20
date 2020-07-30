/*
  
*/

#include <arduino-timer.h>

#define LED_0_PIN 2
#define LED_1_PIN 3

#define SEL_SPRNKLR_BTN A3
#define ADD_TIME_BTN    A4

#define SET_PERIOD 20000         // Allow 20 sec for setting runtime

// 25ms Ticks of LED state
#define TICKS_MIN_H 6
#define TICKS_MIN_L 12
#define TICKS_PAUSE 36

class ProgState {
  public:
    static int minsLeft;
    static bool setState;
    static bool runState;
    static unsigned long runMillis;
  
    static const int minsLeft_init = 0;
    static const bool setState_init = false;
    static const bool runState_init = false;
    static const unsigned long runMillis_init = 0;
        
    void reset() {
      minsLeft = minsLeft_init;
      setState = setState_init;
      runState = runState_init;
      runMillis = runMillis_init;
    }

    // Blinker control will execute this at start of it's cycle
    void detect_runState() {
      if (setState==true && runState==false && millis()>runMillis) {
        setState = false;
        runState = true;
      }
    }
};

ProgState pstate = ProgState();
// need to set these before use in child class
int  ProgState::minsLeft = ProgState::minsLeft_init;
bool ProgState::setState = ProgState::setState_init;
bool ProgState::runState = ProgState::runState_init;
unsigned long ProgState::runMillis = ProgState::runMillis_init;

class BlinkCtrl {

  public:

    BlinkCtrl() {
      num_mins_ticks = TICKS_MIN_H + TICKS_MIN_L;
      ps = ProgState();
      reset();
    }

    void reset() {   // reset public and private vars
      state_mins = 0;
      state_ticks = 0;
    }

    bool set_led() {   // called from timer function lambda wrapper

      ps.detect_runState();
      if (ps.runState == false) return true;

      if (state_ticks == 0) {
        state_ticks = ps.minsLeft*(num_mins_ticks) + TICKS_PAUSE;
        state_mins = ps.minsLeft;
        if (ps.minsLeft > 0) state_mins_ticks = num_mins_ticks;
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
      return true; // continue timer
    }

  private:
    ProgState ps;
    int state_mins;
    int state_ticks;
    int state_mins_ticks;
    int num_mins_ticks;

};

BlinkCtrl blinker = BlinkCtrl();

// --------------------------------------
// Timers
//
auto timer = timer_create_default(); // create a timer with default settings

bool changeMins(void *) {
  if (pstate.minsLeft > 0 && millis() > (pstate.runMillis + 10000) ) pstate.minsLeft--;
  return true;
}

class AnyButton {

  uint8_t button;

  public:
    AnyButton(uint8_t b) {
      button = b;
      ps = ProgState();
      butState = false;
      butReleased = false;
      prevButState = false;
    }

    bool checkButton() {  // called from timer function lambda wrapper
      b_state_0 = b_state_1;
      b_state_1 = b_state_2;
      b_state_2 = digitalRead(button);

      if (butState==false && b_state_0==LOW  && b_state_1==LOW  && b_state_2==LOW)  butState = true;
      if (butState==true  && b_state_0==HIGH && b_state_1==HIGH && b_state_2==HIGH) butReleased = true;
      return true; // continue timer
    }

    virtual void handleButAction() {}

    bool handleButton() {  // called from timer function lambda wrapper

      // Handle condition
      if (butState==true && prevButState==false && butReleased==false) handleButAction();

      // Reset condition
      if (butState==true && butReleased==true) {
        butState = false;
        butReleased = false;
      }

      prevButState = butState;
      return true; // continue timer
    }

  protected:
    ProgState ps;

  private:
    bool butState;
    bool prevButState;
    bool butReleased;
    uint8_t b_state_0;
    uint8_t b_state_1;
    uint8_t b_state_2;

};

class selButton : public AnyButton {
  public:
    selButton():AnyButton(SEL_SPRNKLR_BTN) {}

  private:
    void handleButAction() {
      if (ps.runState == false) {
        digitalWrite(LED_0_PIN, !digitalRead(LED_0_PIN)); // toggle LED 0
        digitalWrite(LED_1_PIN, !digitalRead(LED_0_PIN)); // set to opposite of the other
      }
    }
};

selButton selectBut = selButton();

class addButton : public AnyButton {
  public:
    addButton(BlinkCtrl b):AnyButton(ADD_TIME_BTN) { b_obj = b; }

  private:
    BlinkCtrl b_obj;

    void handleButAction() {

      if (ps.setState == false && ps.runState == false) {
        ps.setState = true;
        ps.runMillis = millis() + SET_PERIOD;
      } else if (ps.runState == true) {
        ps.reset();
        b_obj.reset();
      }

      if ( ps.setState == true && ps.minsLeft < 10 ) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(75);
        digitalWrite(LED_BUILTIN, LOW);
        ps.minsLeft++;
      }      
    }
};

addButton addTimeBut = addButton(blinker);

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  pstate.reset();
  blinker.reset();
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_0_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_0_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(SEL_SPRNKLR_BTN, INPUT_PULLUP);
  pinMode(ADD_TIME_BTN, INPUT_PULLUP);

  timer.every(25, [](void *)->bool{return blinker.set_led();});
  timer.every(30000, changeMins);

  timer.every(20,  [](void *)->bool{return selectBut.checkButton();});
  timer.every(400, [](void *)->bool{return selectBut.handleButton();});

  timer.every(20,  [](void *)->bool{return addTimeBut.checkButton();});
  timer.every(400, [](void *)->bool{return addTimeBut.handleButton();});
}

void loop() {
  timer.tick(); // tick the timer
}
