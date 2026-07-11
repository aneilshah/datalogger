// Button.cpp
#include "global.h"

#include "oled.h"
#include "power.h"
#include "wifiFunc.h"

void processButtonHold(uint32_t hold) {

}

void processLongButtonPressEvent(uint32_t hold) {

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

void processButton(uint32_t hold)
{
    static constexpr uint8_t HOLD_THRESH = 5;
    static uint32_t prevHold = 0;

    if (hold == 0) {
        if (prevHold > 0 && prevHold < HOLD_THRESH) {
          processShortButtonPressEvent();
          Serial.printf("Short Button Press: %u\n", prevHold);
        }
        else if (prevHold >= HOLD_THRESH) {
          processLongButtonPressEvent(hold);
          Serial.printf("Long Button Press: %u\n", prevHold);
        }

        prevHold = 0;
        return;
    }

    if (hold >= HOLD_THRESH)
        processButtonHold(hold);

    prevHold = hold;
}