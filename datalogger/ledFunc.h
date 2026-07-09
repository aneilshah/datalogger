#pragma once

#define LED_ON 1
#define LED_OFF 0

extern uint8_t LED_STATE;

void toggleLED();
void LEDOn();
void LEDOff();
void setLED();
