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
  if (powerMode != POWER_FULL) {
    ledOn();
    oledOn();
    connectWifi();
  }
  powerMode = POWER_FULL;
}

void resumeHalfPowerMode() {
  powerMode = POWER_HALF;
  ledOn();
  oledOn();
}

uint8_t getPowerMode() {
  return powerMode;
}