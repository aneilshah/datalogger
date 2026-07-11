#include "global.h"
#include "ledFunc.h"

uint8_t LED_STATE = LED_OFF;

void toggleLed() {
  LED_STATE = !LED_STATE;
  setLed();
}

void ledOn() {
  LED_STATE = LED_ON;
  setLed();
}

void ledOff() {
  LED_STATE = LED_OFF;
  setLed();
}

void setLed() {
  digitalWrite(LED_PIN, LED_STATE);  // LED
}
