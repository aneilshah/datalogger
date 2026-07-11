// Libraries
#include <event.h>

// Local Files
#include "global.h"
#include "ledFunc.h"
#include "oled.h"
#include "wifiFunc.h"

int testModeADC = 0;
int pumpCount = 0;
Event pumpTestEvent;
long testLogCyc = 0;

#define CYCLE_TIME 30
#define PUMP_ON 1
#define PUMP_OFF 0

int getTestModeADC() {
  return testModeADC;
}

void setTestModeADC(float current) {
  //testModeADC = 0;
}

void initPumpSim() {
  pumpTestEvent.setSec(CYCLE_TIME);
}

float getPumpCurrent(int count) {
  int index = 77 - count;
  float cur = 0.0f;
  if (index < 7) {
    cur = 10.0f + 14.0f * expf(-float(index) / 2.5f);
  }
  else if (index < 66) {
    cur = 9.5f + (float)random(0,101) / 100.0f;
  }
  else { // index >= 66
    float tailIndex = (float)(index - 66);      // 0..11
    cur = 10.0f * (1.0f - tailIndex / 11.0f);   // 10 -> 0
    if (cur < 0.0f) cur = 0.0f;
  }
  return cur;
}

void simulatePump() {
  static int simPumpState = PUMP_OFF;

  if (simPumpState == PUMP_OFF) {
    // Look for Turn On Event
    if (pumpTestEvent.check()) {
      // Serial.println("Test Mode Pump Event: ");
      simPumpState = PUMP_ON;
      pumpCount = 77;
    }
    setTestModeADC(0);
  }
  
  else if (simPumpState == PUMP_ON) {
    // Look for Turn Off Event
    if (pumpCount == 0) {
      int nextEvent = random(CYCLE_TIME, CYCLE_TIME+10);
      pumpTestEvent.setSec(nextEvent);
      Serial.println("Set Next Test Mode Pump On Event: " + String(nextEvent) + " sec");
      simPumpState = PUMP_OFF;
      setTestModeADC(0.0f);   // drop to 0 immediately
    }

    else if (pumpCount > 0) {
      setTestModeADC(getPumpCurrent(pumpCount));
      pumpCount--;
    }
  }
  
  else {
    Serial.println("Invalid Simluated Pump State");
  }
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