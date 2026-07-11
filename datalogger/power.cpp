// Power.cpp
#include "global.h"

#include "power.h"

uint8_t powerMode = POWER_FULL;

void lowPowerModeInit() {
  powerMode = POWER_LOW;
}

void fullPowerMode() {
  powerMode = POWER_FULL;
}

uint8_t getPowerMode() {
  return powerMode;
}