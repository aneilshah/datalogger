#pragma once

// Power Modes
#define POWER_LOW 0
#define POWER_FULL 1
#define POWER_HALF 2

// actions
void lowPowerModeInit();
void resumeFullPowerMode();
void resumeHalfPowerMode();
void wakeUp();

// getters
uint8_t getPowerMode();