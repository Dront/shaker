typedef enum {TIME, SET_TIME_1, SET_TIME_2, INFO, PORTIONS, PREPARING, LAST_PREP, DAYS, SET_DAYS} SystemState;
typedef enum {NOT_PRESSED, PRESSED, SHORT_PRESS, LONG_PRESS, SUPER_LONG_PRESS} ButtonState;
typedef enum {NOT_ROTATED, ROTATED_RIGHT, ROTATED_LEFT} RotaterState;
typedef enum {ENABLED, DISABLED} RelayState;
typedef enum {MOTOR_ENABLED, MOTOR_DISABLED} MotorState;

//*******************************************************************************
class FPSCounter {

#define SECOND 1000

private:
  uint8_t counter;
  unsigned long timer;
  uint8_t result;
  
public:
  FPSCounter() {
    timer = millis();
    result = 0;
  }
  
  inline bool addFrame() {
    ++counter;
    
    unsigned long currentTime = millis();
    if (currentTime - timer > SECOND) {
      timer = currentTime;
      result = counter;
      counter = 0;
      return true;
    }
    
    return false;
  }
  
  inline uint8_t count() {
    return result;
  }
  
};




//*******************************************************************************
class Button{

#define LONG_PRESS_DELAY 1000
#define NEXT_PRESS_DELAY 300
#define SUPER_LONG_PRESS_DELAY 10000

private:
  uint8_t pin;
  ButtonState state;
  unsigned long pressStart;
  unsigned long prevPress;

public:
  Button(const int8_t p) {
    pin = p;
    pinMode(pin, INPUT);
    prevPress = 0;
    //check();
    state = NOT_PRESSED;
  }
  
  inline void check() {
    unsigned long curTime = millis();
    
    if (curTime - prevPress < NEXT_PRESS_DELAY) {
      state = NOT_PRESSED;
      return;
    }
    
    if (digitalRead(pin) == LOW) {
      pressStart = curTime;
      state = PRESSED;
    } else if (state == PRESSED){
      prevPress = curTime;
      unsigned long pressTime = curTime - pressStart;
      if (pressTime > LONG_PRESS_DELAY) {
        state = LONG_PRESS;
      } else {
        state = SHORT_PRESS;
      }
    }
  }
	
  inline void checked() {
    state = NOT_PRESSED;
  }
	
  inline ButtonState getState() {
    return state;
  }
};





//*******************************************************************************
class Rotater {

#define MIN_ROTATION_DELAY 100
  
private:
  uint8_t pinA, pinB;
  RotaterState state = NOT_ROTATED;
  unsigned long previousTime = 0;
  uint8_t previousCode;

  inline uint8_t getCode() {
    uint8_t a = digitalRead(pinA);
    uint8_t num = (a << 1) + digitalRead(pinB);
    return num ^ a;
  }

public:

  Rotater(const int8_t a, const int8_t b){
    previousCode = getCode();
    
    pinA = a;
    pinB = b;

    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
  }
  
  
  bool check(){
    uint8_t currentCode = getCode();
    
    unsigned long currentTime = millis();
    if (currentTime - previousTime > MIN_ROTATION_DELAY) {
      if (currentCode == ((previousCode + 1) % 4)) {
        state = ROTATED_RIGHT;
      } else if (currentCode == ((previousCode - 1) % 4)) {
        state = ROTATED_LEFT;
      }
    
      previousTime = currentTime;
      previousCode = currentCode;
      return true;
    } else {
      previousCode = currentCode;
      return false;
    }
  }

  inline void checked(){
    state = NOT_ROTATED;
  }

  inline RotaterState getState(){
    return state;
  }
};




//*******************************************************************************
class Portion {

#define MAX_PORTIONS 8
#define MIN_PORTIONS 2
#define STD_PORTIONS 3
#define MILK_PER_PORTION 25;

private:
  uint8_t count;
  
  inline bool checkCount(const int num) {
    if (num > MAX_PORTIONS || num < MIN_PORTIONS) {
      return false;
    }
    return true;
  }
  
public:
  
  Portion(const uint8_t num) {
    if (checkCount(num)){
      count = num;
    } else {
      count = STD_PORTIONS;
    }
  }
  
  inline void operator++(int) {
    if (checkCount(count + 1)) {
      count++;
    }
  }
  
  inline void operator--(int) {
    if (checkCount(count - 1)) {
      count--;
    }
  }
  
  inline uint8_t getCount() {
    return count;
  }
  
  inline uint16_t getMilk() {
    return count * MILK_PER_PORTION;
  }
  
};




//*******************************************************************************
class Relay {

private:
  
  RelayState state;
  uint8_t pin1;
  int timerNum = -1;

public:

  Relay(const uint8_t p) {
    pin1 = p;
    
    state = DISABLED;
    
    pinMode(pin1, OUTPUT);
    digitalWrite(pin1, HIGH);
  }
  
  inline void setTimerNum(const int num) {
    timerNum = num;
  }
  
  inline int getTimerNum() {
    return timerNum;
  }
  
  inline void enable() {
    digitalWrite(pin1, LOW);
    state = ENABLED;
  }
  
  inline void disable() {
    digitalWrite(pin1, HIGH);
    state = DISABLED;
  }
  
  inline RelayState getState(){
    return state;
  }
  
};





//*******************************************************************************
class DayCounter {

#define MAX_DAYS 50
#define MIN_DAYS 0
#define STD_DAYS 10  
  
private:
  
  uint8_t count;

  inline bool checkCount(const uint8_t num) {
    if (num < MIN_DAYS || num > MAX_DAYS) {
      return false;
    }
    return true;
  }

public:

  DayCounter(const uint8_t d) {
    if (checkCount(d)) {
      count = d;
    } else {
      count = STD_DAYS;
    }
  }

  inline void operator++(int) {
    if (checkCount(count + 1)) {
      count++;
    }
  }
  
  inline void operator--(int) {
    if (checkCount(count - 1)) {
      count--;
    }
  }
  
  inline uint8_t getCount() {
    return count;
  }

};




//*******************************************************************************
class Motor {

private:
  
  uint8_t pin;
  MotorState state;  

public:

  Motor(const uint8_t p) {
    pin = p;
    
    state = MOTOR_DISABLED;
    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  
  inline void enable() {
    digitalWrite(pin, HIGH);
    state = MOTOR_ENABLED;
  }
  
  inline void disable() {
    digitalWrite(pin, LOW);
    state = MOTOR_DISABLED;
  }
  
  inline MotorState getState(){
    return state;
  }


};




//*******************************************************************************
class Magnet {
  
  private:
    uint8_t pin;
    
  public:
    Magnet (const uint8_t p) {
    	pin = p;
    	pinMode(pin, INPUT);
    }
    
    inline bool enabled() {
    	return digitalRead(pin);
    }

};
