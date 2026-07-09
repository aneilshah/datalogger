#include "global.h"
#include "ledFunc.h"

uint8_t LED_STATE = LED_OFF;

void toggleLED() {
  LED_STATE = !LED_STATE;
  setLED();
}

void LEDOn() {
  LED_STATE = LED_ON;
  setLED();
}

void LEDOff() {
  LED_STATE = LED_OFF;
  setLED();
}

void setLED() {
  digitalWrite(LED_PIN, LED_STATE);  // LED
}
