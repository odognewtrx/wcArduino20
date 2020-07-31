/*
  
*/

#include <arduino-timer.h>

#define LED_0_PIN 2
#define LED_1_PIN 3

#define SPRINKLER_0_PIN 6
#define SPRINKLER_1_PIN 7

#define SEL_SPRNKLR_BTN A3
#define ADD_TIME_BTN    A4

#define SET_PERIOD 20000         // Allow 20 sec for setting runtime

// 25ms Ticks of LED state
#define TICKS_MIN_H 6
#define TICKS_MIN_L 12
#define TICKS_PAUSE 36

class ProgState {
  public:

    ProgState() { setup(); }

    int minsLeft;
    bool setState;
    bool runState;
    unsigned long runMillis;
  
    void setup() {
      minsLeft = 0;
      setState = false;
      runState = false;
      runMillis = 0;
    }

    void reset() { setup(); }
    
    // Blinker control will execute this at start of it's cycle
    void detect_runState() {
      if (setState==true && runState==false && millis()>runMillis) {
        setState = false;
        runState = true;
      }
    }
};

class BlinkCtrl {

  public:

    BlinkCtrl(uint8_t L, uint8_t S0, uint8_t S1) {
      num_mins_ticks = TICKS_MIN_H + TICKS_MIN_L;
      led = L;
      spr_0 = S0;
      spr_1 = S1;
    }

    void setup(ProgState *p) {
      ps = p;
      reset();
    }

    void reset() {   // reset public and private vars
      state_mins = 0;
      state_ticks = 0;
      Serial.print("Turn off sprinklers  ");
      Serial.print(spr_0);
      Serial.print("  ");
      Serial.println(spr_1);
      digitalWrite(spr_0, LOW);
      digitalWrite(spr_1, LOW);
    }

    bool setProgress() {   // called from timer function lambda wrapper

      ps->detect_runState();
      if (ps->runState == false) return true;

      if (ps->runState && ps->minsLeft==0 ) {
        ps->runState = false;
        Serial.print("Turn off sprinklers  ");
        Serial.print(spr_0);
        Serial.print("  ");
        Serial.println(spr_1);
        digitalWrite(spr_0, LOW);
        digitalWrite(spr_1, LOW);
        return true;
      }

      if (state_ticks == 0) {
        state_ticks = ps->minsLeft*(num_mins_ticks) + TICKS_PAUSE;
        state_mins = ps->minsLeft;
        if (ps->minsLeft > 0) state_mins_ticks = num_mins_ticks;
      }

      if (state_ticks > TICKS_PAUSE) {
        if (state_mins > 0) {
          if (state_mins_ticks == num_mins_ticks)   digitalWrite(led, HIGH);
          else if (state_mins_ticks == TICKS_MIN_L) digitalWrite(led, LOW);
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
    ProgState *ps;
    uint8_t spr_0;
    uint8_t spr_1;
    uint8_t led;
    int state_mins;
    int state_ticks;
    int state_mins_ticks;
    int num_mins_ticks;
};

class AnyButton {

  uint8_t button;

  public:

    AnyButton(uint8_t b) { // run by child class constructors
      button = b;
    }

    void setup() {
      butState = false;
      butReleased = false;
      prevButState = false;
      pinMode(button, INPUT_PULLUP);
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
    selButton(uint8_t but):AnyButton(but) {}
    void setup(ProgState *p) {
      ps = p;
      AnyButton::setup();
    }

  private:

    ProgState *ps;
    void handleButAction() {
      if (ps->runState == false) {
        digitalWrite(LED_0_PIN, !digitalRead(LED_0_PIN)); // toggle LED 0
        digitalWrite(LED_1_PIN, !digitalRead(LED_0_PIN)); // set to opposite of the other
      }
    }
};

class addButton : public AnyButton {
  public:
  
    addButton(uint8_t but):AnyButton(but) { }

    void setup(ProgState *p, BlinkCtrl *b) {
      b_obj = b;
      ps = p;
      AnyButton::setup();
    }

  private:
    BlinkCtrl *b_obj;
    ProgState *ps;

    void handleButAction() {

      if (ps->setState == false && ps->runState == false) {
        ps->setState = true;
        ps->runMillis = millis() + SET_PERIOD;
        Serial.println("got here start Set");
      } else
      if (ps->runState == true) {
        ps->reset();
        b_obj->reset();
      }

      if ( ps->setState == true && ps->minsLeft < 10 ) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(75);
        digitalWrite(LED_BUILTIN, LOW);
        ps->minsLeft++;
      }      
    }
};

class CountDown {

  public:
    CountDown(uint8_t L0, uint8_t L1, uint8_t S0, uint8_t S1) {
      led_0 = L0;
      led_1 = L1;
      spr_0 = S0;
      spr_1 = S1;
    }

    void setup(ProgState *p) { ps = p; }

    bool checkTime() {

      if (setState_prev && !ps->setState && ps->runState) {
        // transition from setState to runState
        endTime = millis() + 60000*ps->minsLeft;

        // Turn on sprinkler corresponding to selected LED
        if      (digitalRead(led_0)) {
          Serial.println("Turn on sprinkler 0");
          digitalWrite(spr_0, HIGH);
        }
        else if (digitalRead(led_1)) {
          Serial.println("Turn on sprinkler 1");  
          digitalWrite(spr_1, HIGH);       
        }
        Serial.print("minsLeft: ");
        Serial.println(ps->minsLeft);

      } else if (ps->runState && ps->minsLeft>0 ) {
        // in runState
        unsigned long currTime = millis();
        unsigned long nextMin = endTime - 60000*(ps->minsLeft-1);

        if (currTime >= endTime) {
          Serial.println("minsLeft and set to 0");
          ps->minsLeft = 0;
        }
        else if ( currTime > nextMin ) {
          ps->minsLeft--;
          Serial.print("New minsLeft: ");
          Serial.println(ps->minsLeft);
        }
      }

      setState_prev = ps->setState;
      return true;
    }

  private:
    ProgState *ps;
    uint8_t led_0;
    uint8_t led_1;
    uint8_t spr_0;
    uint8_t spr_1;
    unsigned long endTime;
    bool setState_prev;
};

//------------------------------------------------------------
// Create globally scoped objects
ProgState pstate = ProgState();
BlinkCtrl blinker = BlinkCtrl(LED_BUILTIN, SPRINKLER_0_PIN, SPRINKLER_1_PIN);
selButton selectBut = selButton(SEL_SPRNKLR_BTN);
addButton addTimeBut = addButton(ADD_TIME_BTN);
CountDown countDwnTimer = CountDown(LED_0_PIN, LED_1_PIN, SPRINKLER_0_PIN, SPRINKLER_1_PIN);

auto timer = timer_create_default(); // create a timer with default settings

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_0_PIN, OUTPUT);
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_0_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(SPRINKLER_0_PIN, OUTPUT);
  digitalWrite(SPRINKLER_0_PIN, LOW);
  pinMode(SPRINKLER_1_PIN, OUTPUT);
  digitalWrite(SPRINKLER_1_PIN, LOW);
  
  pstate.setup();
  blinker.setup(&pstate);
  selectBut.setup(&pstate);
  addTimeBut.setup(&pstate, &blinker);   
  countDwnTimer.setup(&pstate);

  timer.every(25, [](void *)->bool{return blinker.setProgress();});

  timer.every(20,  [](void *)->bool{return selectBut.checkButton();});
  timer.every(400, [](void *)->bool{return selectBut.handleButton();});

  timer.every(20,  [](void *)->bool{return addTimeBut.checkButton();});
  timer.every(400, [](void *)->bool{return addTimeBut.handleButton();});

  timer.every(1000, [](void *)->bool{return countDwnTimer.checkTime();});
}

void loop() {
  timer.tick(); // tick the timer
}
