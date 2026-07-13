// Libraries
#include <event.h>

// Local Files
#include "global.h"
#include "ledFunc.h"
#include "oled.h"
#include "wifiFunc.h"

int testModeADC = 0;
long testLogCyc = 0;

#define CYCLE_TIME 30

int getTestModeADC() {
  return testModeADC;
}

void setTestModeADC(float current) {
  //testModeADC = 0;
}

void initLoggerSim() {

}


void simulateLogger() {
  testLogCyc++;
  
  // LED Blinker Routine
  if (wifiRadioOn()) {
    if (testLogCyc % 20 == 0) ledOn();
    else if (testLogCyc % 20 == 10) ledOff();
  }
  else {
    if (testLogCyc % 10 == 0) ledOn();
    else if (testLogCyc % 10 == 1) ledOff();
  }
}

void turnOnWifi() {
  newPopupScreen("WIFI ON", "");
}

void turnOffWifi() {
  newPopupScreen("WIFI OFF", "press button to wake");
}