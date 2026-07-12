// Button.cpp
#include "global.h"
#include "button.h"

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
static uint16_t prevHold = 0;

bool buttonStuck() {return stuckButton;}
bool shortPress() {return buttonState == SHORT_PRESS;}
bool longPress() {return buttonState == LONG_PRESS;}
uint16_t buttonHold() {return holdTime;}

void processButtonHold(uint16_t hold) {
  buttonState = BUTTON_HOLD;
}

void processLongButtonPressEvent(uint16_t hold) {
  buttonState = LONG_PRESS;
  Serial.printf("Long Button Press: %u\n", hold);
} 

void processShortButtonPressEvent(uint16_t hold) {  
  buttonState = SHORT_PRESS;
  Serial.printf("Short Button Press: %u\n", hold);
}

void processButton(uint8_t btnVal)
{
    static constexpr uint16_t HOLD_THRESH = 5;
    static constexpr uint16_t MAX_HOLD = 500;

    //Serial.printf("btnVal: %u  holdTime: %u  prevHold: %u  state: %u\n", btnVal, holdTime, prevHold, buttonState);

    // Update Hold Time
    if (btnVal && holdTime < MAX_HOLD) holdTime++;
    if (!btnVal) holdTime = 0;

    // Check Button Off
    if (holdTime == 0) {
      if (prevHold > 0 && prevHold < HOLD_THRESH) {
        processShortButtonPressEvent(prevHold);
      }
      else if (prevHold >= HOLD_THRESH) {
        processLongButtonPressEvent(prevHold);
      }
      else {
        buttonState = BUTTON_OFF;
      }

      holdTime = 0;
      prevHold = 0;
      return;
    }

    // Check Button Held
    if (holdTime >= HOLD_THRESH) {
      processButtonHold(holdTime);
    }

    // note that we are not setting holdTime until long hold detected
    prevHold = holdTime;
}