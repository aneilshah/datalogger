// Library Files
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "global.h"

// Project Files
#include "button.h"
#include "mode.h"
#include "ntp.h"
#include "oled.h"
#include "power.h"
#include "pumpData.h"
#include "pumpFunc.h"
#include "wifiFunc.h"

// Just in case
#include <stdio.h>
#include <string.h>

// Global Variables
#include "global.h"

// Display
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

// OLED State
static uint8_t oledMode = OLED_OFF;
static uint8_t nextOledMode = OLED_MAIN;
static uint16_t popupTimeout = 0;
static char PopupText[32] = "";
static char PopupDetails[64] = "";
static uint32_t popupTimer = 0;
static uint32_t mainTimer = 0;
static uint32_t mainTimeout = OLED_MODE_NO_TIMEOUT;
static uint32_t oledModeTimer = 0;
static char modalTitle[32] = "";
static bool oledModalEvent = false;

#define MIN_MODE_CYCLE_TIME (5 * LOOPS_PER_SEC)

//-------------------------------------------
// HELPERS
//-------------------------------------------

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setOledMode(uint8_t mode) {
  oledMode = mode;
  oledModeTimer = 0;
}

void clearModalEvent() {
  oledModalEvent = false;
}

bool modalEvent() {
  return oledModalEvent;
}

void initDisplay() {
  display.init();
  setOledMode(OLED_OFF);
  VextON();
}

void updateOLED() {
  static unsigned int offsetTimer = 0;
  static unsigned int horOffset = 0;
  static unsigned int vertOffset = 0;
  static unsigned int vertOffsetMain = 0;

  // Local scratch buffers
  char line0[48];
  char line1[48];
  char line2[48];
  char line3[48];
  char line4[48];
  char line5[48];

  horOffset = (offsetTimer / (60 * LOOPS_PER_SEC)) % 10;  // Every 60 seconds, span = 10 pixels  

  if (oledMode == OLED_MAIN) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Line 0: HEADER
    if (TEST_MODE) snprintf(line0, sizeof(line0), "DATA LOG %s T", APP_VERSION);
    else           snprintf(line0, sizeof(line0), "DATA LOG %s", APP_VERSION);
    display.drawString(horOffset, vertOffsetMain, line0);

    // Line 1: Wifi
    if (wifiRadioOn())
      snprintf(line1, sizeof(line1), "WIFI: %s", CONN_STATUS);
    else
      snprintf(line1, sizeof(line1), "WIFI: RADIO OFF");
    display.drawString(horOffset, 10 + vertOffsetMain, line1);

    // Line 2: CPH / CPD
    float avgCycleMin = Pump.getAvgCycleMin();
    float cph = 0.0f;
    int cpd = 0;

    if (avgCycleMin > 0.1f) {
      cph = 60.0f / avgCycleMin;
      cpd = (int)(cph * 24.0f);
    }
    char curText[12];
    Pump.getPumpCurText(curText, sizeof(curText));

    snprintf(line2, sizeof(line2), "CPH: %.1f  CPD: %d", cph, cpd);
    display.drawString(horOffset, 20 + vertOffsetMain, line2);


    // Line 3: Gallons - Current
    int gallonsPerDay = 0;
    if (avgCycleMin > 0.1f) {
      gallonsPerDay = (int)(5 * 60 * 24 / avgCycleMin);
    }
    if (avgCycleMin <= 0.1f) {
      snprintf(line3, sizeof(line3), "GPD: --  I: %s", curText);
    } else {
      snprintf(line3, sizeof(line3), "GPD: %d  I: %s", gallonsPerDay, curText);
    }
    display.drawString(horOffset, 30 + vertOffsetMain, line3);


    /////////////////////////////////////////
    // Line 4: Revolving Line
    /////////////////////////////////////////
    const float TimePerStage = 2.0; // Seconds
    const int Stages = 3;
    const int TotalTime = LOOPS_PER_SEC * TimePerStage * Stages;
    const int TimePerStageLoops = int(TimePerStage * LOOPS_PER_SEC);

    if (LOOP_COUNT % TotalTime < TimePerStageLoops) {
      // Pct ON / OFF
      snprintf(line4, sizeof(line4), "<RESERVED 1>");
    }
    else if (LOOP_COUNT % TotalTime < 2 * TimePerStageLoops) {
      snprintf(line4, sizeof(line4), "<RESERVED 2>");
    }
    else {
      snprintf(line4, sizeof(line4), "<RESERVED 3>");

    }

    display.drawString(horOffset, 40 + vertOffsetMain, line4);

    // Line 5: On Time / Clock
    float hours = offsetTimer / 36000.0f;
    if (hours < 24.0f) {
      snprintf(line5, sizeof(line5), "UP:%.1fh | CLK:%s", hours, getClock());
    } else {
      float days = hours / 24.0f;
      snprintf(line5, sizeof(line5), "UP:%.1fd | CLK:%s", days, getClock());
    }
    display.drawString(horOffset, 50 + vertOffsetMain, line5);

    // write the buffer to the display
    display.display();

    // Update timers and change state as needed
    if (mainTimer > 0) mainTimer--;
    if (mainTimeout != OLED_MODE_NO_TIMEOUT && mainTimer == 0) {
      setOledMode(OLED_MINIMIZED);  
    }
  }

  else if (oledMode == OLED_MINIMIZED) {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;

    uint32_t gallonsPerDay = (uint32_t)(5 * 60 * 24 / deltaMin);

    char cycMin[10];
    snprintf(cycMin, sizeof(cycMin), "%.1f", deltaMin);

    const unsigned int CYCLE_COUNT = 8;
    unsigned int mode = (offsetTimer / MIN_MODE_CYCLE_TIME) % CYCLE_COUNT;

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    if (mode == 0) {
      if (TEST_MODE) snprintf(line0, sizeof(line0), "*TEST MODE* %s", APP_VERSION);
      else           snprintf(line0, sizeof(line0), "DATA APP %s", APP_VERSION);
      display.drawString(horOffset, vertOffset, line0);
    }
    else if (mode == 1) {
      snprintf(line2, sizeof(line2), "ON: %u%c [%u%c]",
              (unsigned)(offsetTimer / LOOPS_PER_SEC), 's',
              (unsigned)(offsetTimer / 864000), 'd');
      display.drawString(horOffset, vertOffset, line2);
    }
    else if (mode == 2) {
      snprintf(line2, sizeof(line2), "CYC: %sm %ld GPD", cycMin, gallonsPerDay);
      display.drawString(horOffset, vertOffset, line2);
    }
    else if (mode == 3) {
      snprintf(line2, sizeof(line2), "CLOCK: %s", getClock());
      display.drawString(horOffset, vertOffset, line2);
    }
    else if (mode == 4) {
      snprintf(line2, sizeof(line2), "DATE: %s", getDate());
      display.drawString(horOffset, vertOffset, line2);
    }
    else if (mode == 5) {
      snprintf(line2, sizeof(line2), "YEAR: %s", getYearStr());
      display.drawString(horOffset, vertOffset, line2);
    }
    else if (mode == 6) {
      snprintf(line2, sizeof(line2), "Wifi: %s", CONN_STATUS);
      display.drawString(horOffset, vertOffset, line2);
    }
    else {
      display.drawString(horOffset, vertOffset, "Press TOP Button");
    }

    if (offsetTimer % (120 * LOOPS_PER_SEC) == 0) vertOffset = random(0, 50);

    display.display();
  }

  else if (oledMode == OLED_MODAL) {
    uint16_t countdown = 0;
    
    if (buttonHold() < OLED_HOLD_TIMEOUT) 
      countdown = OLED_HOLD_TIMEOUT - buttonHold();

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Status Bar
    snprintf(line1, sizeof(line1), modalTitle);
    display.drawString(horOffset, 10 + vertOffset, line1);

    snprintf(line3, sizeof(line3), "HOLD to Enter (%u)", countdown);
    display.drawString(horOffset, 30 + vertOffset, line3);

    snprintf(line4, sizeof(line4), "Press to Cancel");
    display.drawString(horOffset, 40 + vertOffset, line4);
    display.display();

    if (countdown == 0) {
      if (!oledModalEvent)
        Serial.println("MODAL EVENT");
      oledModalEvent = true;
    }
  } 

  else if (oledMode == OLED_POPUP) {
    updatePopupScreen();
  }

  oledModeTimer++;    // resets with mode change
  offsetTimer++;  // doesnt reset with modes
}

void displayPopupScreen(const char* text, const char* details, uint16_t timeout, uint8_t nextMode) {
  strncpy(PopupText, text, sizeof(PopupText) - 1);
  PopupText[sizeof(PopupText) - 1] = '\0';

  strncpy(PopupDetails, details, sizeof(PopupDetails) - 1);
  PopupDetails[sizeof(PopupDetails) - 1] = '\0';

  popupTimer = 0;
  popupTimeout = timeout;
  nextOledMode = nextMode;
  setOledMode(OLED_POPUP);
  updatePopupScreen();
}

void updatePopupScreen() {
  if (oledMode == OLED_POPUP) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, PopupText);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 16, PopupDetails);

    // write the buffer to the display
    display.display();

    if (popupTimeout && (getModeTimer() > popupTimeout * LOOPS_PER_SEC)) {
      setOledMode(nextOledMode);
    }

    popupTimer++;
  }
}

void newPopupScreen(const char* text, const char* details, uint16_t timeout, uint8_t nextMode) {
  popupTimeout = timeout;
  nextOledMode = nextMode;
  displayPopupScreen(text, details, timeout, nextMode);
  updatePopupScreen();
}

void oledMain(uint32_t duration) {
  setOledMode(OLED_MAIN);
  mainTimer = duration;
  mainTimeout = duration;
}

void oledMinimized() {
  setOledMode(OLED_MINIMIZED);
}

void oledBlank() {
  setOledMode(OLED_OFF);
}

void oledOff() {
  display.displayOff();
}

void oledOn() {
  display.displayOn();
}

void oledModal(const char* title) {
  strncpy(modalTitle, title, sizeof(modalTitle) - 1);
  clearModalEvent();
  setOledMode(OLED_MODAL);
}

void oledPauseLogger() {
  setOledMode(OLED_PAUSE_LOGGER);
}


uint8_t getOledMode() {
  return oledMode;
}
