// Power.cpp
#include "global.h"

#include "ledFunc.h"
#include "oled.h"
#include "power.h"
#include "wifiFunc.h"

uint8_t powerMode = POWER_FULL;

void lowPowerModeInit() {
  powerMode = POWER_LOW;

  disconnectWifi();
  oledOff();
  ledOff();
}

void wakeUp() {

}

void resumeFullPowerMode() {
  powerMode = POWER_FULL;
  ledOn();
  oledOn();
  oledMain();
  connectWifi();
}

void resumeHalfPowerMode() {
  powerMode = POWER_HALF;
  ledOn();
  oledOn();
  oledHalfPower();
}

uint8_t getPowerMode() {
  return powerMode;
}