#include <Arduino.h>
#include <Time.h>
#include <glcd.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/fixednums15x31.h>
#include <fonts/fixednums8x16.h>
#include <fonts/Arial14.h>
#include <fonts/Arial_bold_14.h>

#include "ShakerUtils.h"
#include "SimpleTimer.h"


Rotater enc(2, 3); 
Button btn1(19);
Button btn2(20);
Relay pump(22);
Relay heater(23);
Motor motor(24);
Magnet magnet(25);
OneWire oneWire(26);

#define MAGNET_CHECK_DELAY 10
#define MAX_WATER_TEMP 30
#define MIN_WATER_TEMP 10
DallasTemperature therm(&oneWire);

FPSCounter fps;
SimpleTimer timer;
uint8_t prepTimerNum = -1;
SystemState sysState = TIME;

#define TIME_FOR_MOTOR 1000
#define TIME_FOR_HEATER 1200
#define TIME_FOR_PORTION 2900

Portion portions(3);
DayCounter days(5);
unsigned long prepTimer;

#define LOOP_TIME 25
unsigned long previousLoopTime;

#define TIME_FOR_DONE_SCREEN 2000
#define TIME_FOR_TEMP_SCREEN 2000

#define CLEAR_SCREEN_TIME 2000
unsigned long clearScreenTime;
uint8_t clearScreen = false;

//for setting time and expire date
time_t tmp_time;
DayCounter tmp_day(5);

void setup() {
  unsigned long curTime = millis();
  
  prepTimer = curTime;
  previousLoopTime = curTime;
  clearScreenTime = curTime;
  
  attachInterrupt(4, button1Interrupt, CHANGE);
  attachInterrupt(3, button2Interrupt, CHANGE);
  attachInterrupt(0, encoderInterrupt, CHANGE);
  attachInterrupt(1, encoderInterrupt, CHANGE);
  
  GLCD.Init();
  therm.begin();
  
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
          tmp_time += 3600;
          break;
        
        case SET_TIME_2:
          tmp_time += 60;
          break; 
          
        case LAST_PREP:
          switchState(DAYS);
          break;
          
        case DAYS:
          switchState(TIME);
          break;
          
        case SET_DAYS:
          tmp_day++;
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
          tmp_time -= 3600;
          break;
        
        case SET_TIME_2:
          tmp_time -= 60;
          break; 
        
        case DAYS:
          switchState(LAST_PREP);
          break;
          
        case LAST_PREP:
          switchState(TIME);
          break;
        
        case SET_DAYS:
          tmp_day--;
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
          setTime(tmp_time);
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
          days = tmp_day;
          switchState(DAYS);
          break;
        
        default:
          break;
      }
      break;
      
    case LONG_PRESS:
      switch(sysState){
        case TIME:
          tmp_time = now();
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
          
        case PREPARING:
          stopPreparing();
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

void stopPreparing() {
  for (int i = 0; i < timer.MAX_TIMERS; ++i) {
    if (timer.isEnabled(i)) {
      timer.deleteTimer(i);
    }
  }
  
  heater.disable();
  pump.disable();
  //ждать пока магнит тикнет
  motor.disable();
}

float getTemperature() {
  therm.requestTemperatures();
  return therm.getTempCByIndex(0);
}

void toTimeScreen(){
  clearScreen = true;
  switchState(TIME);
}

void toDoneScreen(){
  clearScreen = true;
  pump.disable();
  
  timer.deleteTimer(heater.getTimerNum());
  heater.disable();
  
  prepTimer = millis();
  
  switchState(DONE_PREPARING);
  unsigned int time = TIME_FOR_DONE_SCREEN;
  timer.setTimeout(time, toTimeScreen);
}

void toggleHeater() {
  if (heater.getState() == ENABLED) {
    heater.disable();
  } else {
    heater.enable();
  }
}

void motorWork() {
  if (magnet.getCount() == portions.getCount()) {
    timer.deleteTimer(magnet.getTimerNum()); 
    motor.disable();
    
    unsigned int time = (portions.getCount() - 1) * TIME_FOR_PORTION;
    timer.setTimeout(time, toDoneScreen);
    pump.enable();
    
    time = TIME_FOR_HEATER;
    uint8_t num = (portions.getCount() - 1) * 2;
    uint8_t timerNum = timer.setTimer(time, toggleHeater, num);
    heater.setTimerNum(timerNum);
    heater.enable();
  }
  
  motor.toggle();
}

void fillFirstPortion() {
  pump.disable();
  //unsigned int time = TIME_FOR_MOTOR * portions.getCount();
  //timer.setTimeout(time, motorWork);
  
  uint8_t timerNum = timer.setInterval(MAGNET_CHECK_DELAY, motorWork);
  magnet.setTimerNum(timerNum);
  magnet.zeroCount();
  motor.enable();
}

void heatFirstPortion() {
  heater.disable();
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

unsigned int countTimeForPrep() {
  unsigned int timeForHeater = TIME_FOR_HEATER;
  unsigned int timeForWater = TIME_FOR_PORTION * portions.getCount();
  unsigned int timeForMotor = TIME_FOR_MOTOR * portions.getCount();
  return timeForHeater + timeForWater + timeForMotor + 1000;
}

void cook() {
  float temp = getTemperature();
  Serial.println(temp);
  
  if (temp < MAX_WATER_TEMP && temp > MIN_WATER_TEMP) {
    unsigned int time = countTimeForPrep();
    prepTimerNum = timer.setTimeout(time, doNothing);
    
    time = TIME_FOR_HEATER;
    timer.setTimeout(time, preheat); 
    heater.enable();
    
    switchState(PREPARING);
  } else {
    unsigned int time = TIME_FOR_TEMP_SCREEN;
    timer.setTimeout(time, toTimeScreen);
    switchState(WRONG_TEMP);
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
  GLCD.GotoXY(25, 16);
  //GLCD.CursorTo(2, 0);
  uint8_t h = hour();
  if (h < 10) {
    GLCD.print(0);
  }
  GLCD.print(h);
  GLCD.print(":");
  
  uint8_t m = minute();
  if (m < 10) {
    GLCD.print(0);
  }
  GLCD.print(m);
  
  static int count = 0;
  
  if (!magnet.enabled()) {
  //  Serial.println(count++);
  }
}

void showPortions() {
  //GLCD.CursorTo(4, 1);
  //GLCD.print(portions.getMilk());
  //GLCD.print(" ml.  ");
  GLCD.CursorTo(3, 2);
  GLCD.print(portions.getCount());
  GLCD.print(" portions  ");
}

void showHotWater() {
  GLCD.CursorTo(2, 1);
  
  int temp = (int)therm.getTempCByIndex(0);
  if (temp > MAX_WATER_TEMP) {
    GLCD.print("Water is too hot");
  } else {
    GLCD.print("Water is too cold");
  }

  GLCD.CursorTo(3, 2);
  GLCD.print("Temp: ");
  GLCD.print(temp);
  GLCD.print(" C");
}

void showPreparing() {
  GLCD.CursorTo(3, 1);
  GLCD.print("Processing...");
  unsigned long remainingTime = timer.getRemainingTime(prepTimerNum) / 1000;
  GLCD.CursorTo(0, 2);
  GLCD.print("remaining time: ");
  if (remainingTime < 10) {
    GLCD.print(0);
  }
  GLCD.print(remainingTime);
}

void showDonePreparing() {
  GLCD.CursorTo(4, 1);
  GLCD.print("Done!");
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
  GLCD.CursorTo(1, 1);
  uint8_t d = days.getCount();
  GLCD.print("Expire in ");
  GLCD.print(d);
  GLCD.print(" days.    ");
}

void showSetDays() {
  
  static bool clear = false;
  
  uint8_t d = tmp_day.getCount();
  GLCD.CursorTo(2, 1);
  GLCD.print("Days count: ");
  GLCD.CursorTo(6, 2);
  
  if ((millis() / 500) % 2) {
    GLCD.print("");
    GLCD.print(d);
    clear = true;
  } else if (clear) {
    GLCD.ClearScreen();
    clear = false;
  }
}

void showLastPrep() {
  GLCD.CursorTo(2, 1);
  GLCD.print("Since feeding:   ");
  
  unsigned long diff = (millis() - prepTimer) / 1000;
  unsigned long hours = diff / 3600;
  uint16_t mins = diff - hours * 3600;
  uint8_t sec = mins % 60;
  mins /= 60;
  GLCD.CursorTo(3, 2);
  
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
  //GLCD.CursorTo(4, 1);
  GLCD.GotoXY(25, 16);
  static bool clear = false;
  
  unsigned int h = hour(tmp_time);
  if ((millis() / 500) % 2) {
    //GLCD.SetFontColor(WHITE);
    if (h < 10) {
      GLCD.print(0);
    }
    GLCD.print(h);
    clear = true;
  } else {
    if (clear) {
      GLCD.ClearScreen();
      clear = false;
    }
    GLCD.print("   ");
  }
  
  unsigned int m = minute(tmp_time);
  //GLCD.CursorTo(6, 1);
  GLCD.GotoXY(56, 16);
  GLCD.print(":");
  if (m < 10) {
    GLCD.print(0);
  }
  GLCD.print(m);
}

void showSetTime2() {
  //GLCD.CursorTo(4, 1);
  GLCD.GotoXY(25, 16);
  
  static bool clear = false;
  
  unsigned int h = hour(tmp_time);
  //GLCD.print("        ");
  if (h < 10) {
    GLCD.print(0);
  }
  GLCD.print(h);
  GLCD.print(":");
  
  unsigned int m = minute(tmp_time);
  if ((millis() / 500) % 2) {
    if (m < 10) {
      GLCD.print(0);
    }
    GLCD.print(m);
    clear = true;  
  } else if (clear) {
    GLCD.ClearScreen();
    clear = false;
  }
}

void loop() {
  magnet.update();
  timer.run();
  
  unsigned long currentTime = millis();
  if (currentTime - previousLoopTime < LOOP_TIME) {
    return;
  }
  previousLoopTime = currentTime;
  
  /*if (currentTime - clearScreenTime > CLEAR_SCREEN_TIME) {
    clearScreen = true;
    clearScreenTime = currentTime;
  }*/
  
  if (clearScreen) {
    GLCD.ClearScreen();
    clearScreen = false;
  }
  
  GLCD.SelectFont(Arial_bold_14);
  switch (sysState) {
    case INFO:
      GLCD.SelectFont(System5x7);
      showInfo();
      break;
      
    case TIME:
      GLCD.SelectFont(fixednums15x31);
      showTime();
      break;
      
    case SET_TIME_1:
      GLCD.SelectFont(fixednums15x31);
      showSetTime1();
      break;
    
    case SET_TIME_2:
      GLCD.SelectFont(fixednums15x31);
      showSetTime2();
      break;
    
    case PORTIONS:
      showPortions();
      break;
    
    case WRONG_TEMP:
      showHotWater();
      break;
      
    case PREPARING:
      showPreparing();
      break;
    
    case DONE_PREPARING:
      showDonePreparing();
      break;
    
    case DAYS:
      showDays();
      break;
    
    case SET_DAYS:
      showSetDays();
      break;
      
    case LAST_PREP:
      showLastPrep();
      break;
    
    default:
      break;
  }
}
