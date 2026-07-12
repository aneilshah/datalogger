// Button.cpp
#include "global.h"

#include "oled.h"
#include "power.h"
#include "wifiFunc.h"

#define BUTTON_OFF 0
#define SHORT_PRESS 1
#define LONG_PRESS 2
#define BUTTON_HOLD 3

static uint8_t buttonState = BUTTON_OFF;
static bool stuckButton = false;
static uint16_t holdTime = 0;

bool buttonStuck() {return stuckButton;}
bool shortPress() {return buttonState == SHORT_PRESS;}
bool longPress() {return buttonState == LONG_PRESS;}
uint16_t buttonHold() {return holdTime;}

void processButtonHold(uint16_t hold) {

}

void processLongButtonPressEvent(uint16_t hold) {

} 

void processShortButtonPressEvent() {
  // Toggle Power Modes
  if (getPowerMode() == POWER_FULL && getOledMode() == OLED_MAIN) {
    oledLowPower();
  } 

  else if (getPowerMode() == POWER_FULL && getOledMode() == OLED_LOW_POWER) {
    oledMain();
  } 
  
  else if (getPowerMode() == POWER_HALF) {
    resumeFullPowerMode();
  } 
  
  // Low Power
  else {
    resumeHalfPowerMode();
  }
}

void processButton(uint16_t hold)
{
    static constexpr uint16_t HOLD_THRESH = 5;
    static uint16_t prevHold = 0;

    // Button Off
    if (hold == 0) {
      if (prevHold > 0 && prevHold < HOLD_THRESH) {
        buttonState = SHORT_PRESS;
        processShortButtonPressEvent();
        Serial.printf("Short Button Press: %u\n", prevHold);
      }
      else if (prevHold >= HOLD_THRESH) {
        buttonState = LONG_PRESS;
        processLongButtonPressEvent(hold);
        Serial.printf("Long Button Press: %u\n", prevHold);
      }
      else {
        buttonState = BUTTON_OFF;
      }

      holdTime = 0;
      prevHold = 0;
      return;
    }

    // Button Held
    if (hold >= HOLD_THRESH) {
      buttonState = BUTTON_HOLD;
      holdTime = hold;
      processButtonHold(hold);
    }

    // note that we are not setting holdTime until long hold detected
    prevHold = hold;
}