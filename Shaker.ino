#include <Arduino.h>
#include <Time.h>
#include <glcd.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/fixednums15x31.h>
#include <fonts/fixednums8x16.h>
#include <fonts/Arial14.h>

#include "ShakerUtils.h"
#include "SimpleTimer.h"

Rotater enc(2, 3); 
Button btn1(19);
Button btn2(20);
Relay heater(23);
Relay pump(22);
Magnet magnet(25);
Motor motor(24);

FPSCounter fps;
SimpleTimer timer;
uint8_t prepTimerNum = -1;
SystemState sysState = TIME;

#define TIME_FOR_MOTOR 1000
#define TIME_FOR_HEATER 1500

#define TIME_FOR_PORTION 3000
Portion portions(3);
DayCounter days(5);
unsigned long prepTimer = millis();


#define LOOP_TIME 25
unsigned long previousLoopTime = millis();

uint8_t clearScreen = false;

void setup() {
  attachInterrupt(4, button1Interrupt, CHANGE);
  attachInterrupt(3, button2Interrupt, CHANGE);
  attachInterrupt(0, encoderInterrupt, CHANGE);
  attachInterrupt(1, encoderInterrupt, CHANGE);
  
  GLCD.Init();
  
  setTime(22, 51, 30, 24, 11, 14);
  
  Serial.begin(9600);
}

void switchState(SystemState newState) {
  clearScreen = true;
  if (sysState == newState) {
    return;
  }
  
  sysState = newState;
}

void encoderInterrupt() {
  if (!enc.check()) {
    return;
  }
  
  switch (enc.getState()){
    case ROTATED_RIGHT:
      switch(sysState) {
        case PORTIONS:
          portions++;
          enc.checked();
          break;
          
        case TIME:
          switchState(LAST_PREP);
          break;
          
        case SET_TIME_1:
          adjustTime(3600);
          break;
        
        case SET_TIME_2:
          adjustTime(60);
          break; 
          
        case LAST_PREP:
          switchState(DAYS);
          break;
          
        case DAYS:
          switchState(TIME);
          break;
          
        case SET_DAYS:
          days++;
          enc.checked();
          break;
          
        default:
          break;
      }
      break;
      
    case ROTATED_LEFT:
      switch(sysState) {
        case PORTIONS:
          portions--;
          enc.checked();
          break;
        
        case TIME:
          switchState(DAYS);
          break;
        
        case SET_TIME_1:
          adjustTime(-3600);
          break;
        
        case SET_TIME_2:
          adjustTime(-60);
          break; 
        
        case DAYS:
          switchState(LAST_PREP);
          break;
          
        case LAST_PREP:
          switchState(TIME);
          break;
        
        case SET_DAYS:
          days--;
          enc.checked();
          break;
          
          
        default:
          break;
      }
      break;
      
    default:
      break;
  }
}

void toggleHeater() {
  if (heater.getState() == ENABLED) {
    heater.disable();
  } else {
    heater.enable();
  }
}

void motorWork() {
  motor.disable();
  
  unsigned int time = (portions.getCount() - 1) * TIME_FOR_PORTION;
  timer.setTimeout(time, toTimeScreen);
  pump.enable();
  
  time = TIME_FOR_HEATER;
  uint8_t num = (portions.getCount() - 1) * 2;
  uint8_t timerNum = timer.setTimer(time, toggleHeater, num);
  heater.setTimerNum(timerNum);
  heater.enable();
}

void fillFirstPortion() {
  pump.disable();
  
  unsigned int time = TIME_FOR_MOTOR * portions.getCount();
  timer.setTimeout(time, motorWork);
  motor.enable();
}

void heatFirstPortion() {
  heater.disable();
}

unsigned int countTimeForPrep() {
  unsigned int timeForHeater = TIME_FOR_HEATER;
  unsigned int timeForWater = TIME_FOR_PORTION * portions.getCount();
  unsigned int timeForMotor = TIME_FOR_MOTOR * portions.getCount();
  return timeForHeater + timeForWater + timeForMotor;
}

void doNothing(){}

void preheat() {
  heater.disable();
  
  unsigned int time = TIME_FOR_HEATER; 
  timer.setTimeout(time, heatFirstPortion); 
  heater.enable();
  
  time = TIME_FOR_PORTION;
  timer.setTimeout(time, fillFirstPortion); 
  pump.enable();
}

void cook() {
  unsigned int time = countTimeForPrep();
  prepTimerNum = timer.setTimeout(time, doNothing);
  
  time = TIME_FOR_HEATER;
  timer.setTimeout(time, preheat); 
  heater.enable();
  
  switchState(PREPARING);
}

void toTimeScreen(){
  clearScreen = true;
  pump.disable();
  
  timer.deleteTimer(heater.getTimerNum());
  heater.disable();
  prepTimer = millis();
  switchState(TIME);
}

void button1Interrupt() {
  btn1.check();
  switch (btn1.getState()) {
    case SHORT_PRESS:
      switch (sysState) {
        case TIME:
          switchState(PORTIONS);
          break;
          
        case SET_TIME_1:
          switchState(SET_TIME_2);
          break;
          
        case SET_TIME_2:
          switchState(TIME);
          break;
          
        case PORTIONS:
          cook();
          break;
        
        case LAST_PREP:
          switchState(PORTIONS);
          break;
          
        case DAYS:
          switchState(PORTIONS);
          break;
         
        case SET_DAYS:
          switchState(DAYS);
          break;
        
        default:
          break;
      }
      break;
      
    case LONG_PRESS:
      switch(sysState){
        case TIME:
          switchState(SET_TIME_1);
          break;
        
        case DAYS:
          switchState(SET_DAYS);
          break;
          
        case LAST_PREP:
          prepTimer = millis();
          break;
        
        default:
          break;  
      }
      break;
    
      
    default:
      break;
  }
}

void button2Interrupt() {
  btn2.check();
  
  switch (btn2.getState()) {
    case SHORT_PRESS:
      
      switch(sysState) {
        case SET_TIME_1:
          switchState(TIME);
          break;
          
        case SET_TIME_2:
          switchState(SET_TIME_1);
          break;
        
        case PORTIONS:
          switchState(TIME);
          break;
          
        case SET_DAYS:
          switchState(DAYS);
          break;
        
        default:
          break;
      }
      break;
      
    case LONG_PRESS:
      //switchState(INFO);
      break;
      
    default:
      break;
  }
}

void processEncoder() {  
  GLCD.CursorTo(0, 3);
  switch (enc.getState()) {
    case NOT_ROTATED:
      GLCD.print("enc not rotated  ");
      break;
      
    case ROTATED_RIGHT:
      GLCD.print("enc rotated right  ");
      break;
      
    case ROTATED_LEFT:
      GLCD.print("enc rotated left  ");
      break;
      
    default:
      break;
  }
}

void processButton(const uint8_t btnNumber) {
  ButtonState btnState = (btnNumber == 1) ? btn1.getState() : btn2.getState(); 
  GLCD.CursorTo(0, btnNumber);
  switch (btnState) {
    case NOT_PRESSED:
      GLCD.print("not pressed  ");
      break;
      
    case PRESSED:
      GLCD.print("pressed  ");
      break;
      
    case LONG_PRESS:
      GLCD.print("long press  ");
      break;
      
    case SHORT_PRESS:
      GLCD.print("short press  ");
      break;
      
    default:
      break;
  }
}

void showTime() {
  GLCD.CursorTo(5, 1);
  uint8_t h = hour();
  if (h < 10) {
    GLCD.print(0);
  }
  GLCD.print(hour());
  GLCD.print(":");
  
  uint8_t m = minute();
  if (m < 10) {
    GLCD.print(0);
  }
  GLCD.print(minute());
  
  static int count = 0;
  
  if (!magnet.enabled()) {
    Serial.println(count++);
  }
}

void showPortions() {
  GLCD.CursorTo(4, 1);
  GLCD.print(portions.getMilk());
  GLCD.print(" ml.  ");
  GLCD.CursorTo(3, 2);
  GLCD.print(portions.getCount());
  GLCD.print(" portions  ");
}

void showPreparing() {
  GLCD.CursorTo(3, 1);
  GLCD.print("Processing...");
  unsigned long remainingTime = timer.getRemainingTime(prepTimerNum) / 1000;
  GLCD.CursorTo(2, 2);
  GLCD.print("remaining time: ");
  if (remainingTime < 10) {
    GLCD.print(0);
  }
  GLCD.print(remainingTime);
}

void showInfo() {
  GLCD.CursorTo(7, 0);
  GLCD.print("INFO  ");
      
  processButton(1);
  processButton(2);
  processEncoder();
  
  fps.addFrame();
  GLCD.CursorTo(0, 5);
  GLCD.print("FPS: ");
  GLCD.print(fps.count());
}

void showDays() {
  GLCD.CursorTo(2, 1);
  uint8_t d = days.getCount();
  GLCD.print("Expire in ");
  GLCD.print(d);
  GLCD.print(" days.    ");
}

void showSetDays() {
  uint8_t d = days.getCount();
  GLCD.CursorTo(2, 1);
  GLCD.print("Choose days count: ");
  GLCD.CursorTo(6, 2);
  
  if ((millis() / 500) % 2) {
    GLCD.print("");
    GLCD.print(d);
  } else {
    GLCD.print("         ");
  }
}

void showLastPrep() {
  GLCD.CursorTo(1, 1);
  GLCD.print("Time since feeding:   ");
  
  unsigned long diff = (millis() - prepTimer) / 1000;
  unsigned long hours = diff / 3600;
  uint16_t mins = diff - hours * 3600;
  uint8_t sec = mins % 60;
  mins /= 60;
  GLCD.CursorTo(5, 2);
  
  if (hours < 10) {
    GLCD.print(0);
  }
  GLCD.print(hours);
  GLCD.print(':');
  
  if (mins < 10) {
    GLCD.print(0);
  }
  GLCD.print(mins);
  GLCD.print(':');
  
  if (sec < 10) {
    GLCD.print(0);
  }
  GLCD.print(sec);
}

void showSetTime1() {
  GLCD.CursorTo(4, 1);
  
  static bool clear = false;
  
  if ((millis() / 500) % 2) {
    GLCD.print("        ");
    if (hour() < 10) {
      GLCD.print(0);
    }
    GLCD.print(hour());
    clear = true;
  } else {
    if (clear) {
      GLCD.ClearScreen();
      clear = false;
    }
  }
  
  GLCD.CursorTo(6, 1);
  GLCD.print(":");
  if (minute() < 10) {
    GLCD.print(0);
  }
  GLCD.print(minute());
}

void showSetTime2() {
  GLCD.CursorTo(4, 1);
  
  static bool clear = false;
  
  GLCD.print("        ");
  if (hour() < 10) {
    GLCD.print(0);
  }
  GLCD.print(hour());
  GLCD.print(":");
  
  if ((millis() / 500) % 2) {
    if (minute() < 10) {
      GLCD.print(0);
    }
    GLCD.print(minute());
    clear = true;  
  } else {
    if (clear) {
      GLCD.ClearScreen();
      clear = false;
    }
  }
}

void loop() {
  timer.run();
  
  unsigned long currentTime = millis();
  if (currentTime - previousLoopTime < LOOP_TIME) {
    return;
  }
  previousLoopTime = currentTime;
  
  if (clearScreen) {
    GLCD.ClearScreen();
    clearScreen = false;
  }
  
  switch (sysState) {
    case INFO:
      GLCD.SelectFont(System5x7);
      showInfo();
      break;
      
    case TIME:
      GLCD.SelectFont(Arial_14);
      showTime();
      break;
      
    case SET_TIME_1:
      GLCD.SelectFont(Arial_14);
      showSetTime1();
      break;
    
    case SET_TIME_2:
      GLCD.SelectFont(Arial_14);
      showSetTime2();
      break;
    
    case PORTIONS:
      GLCD.SelectFont(Arial_14);
      showPortions();
      break;
      
    case PREPARING:
      GLCD.SelectFont(Arial_14);
      showPreparing();
      break;
    
    case DAYS:
      GLCD.SelectFont(Arial_14);
      showDays();
      break;
    
    case SET_DAYS:
      GLCD.SelectFont(Arial_14);
      showSetDays();
      break;
      
    case LAST_PREP:
      GLCD.SelectFont(Arial_14);
      showLastPrep();
      break;
    
    default:
      break;
  }
}
