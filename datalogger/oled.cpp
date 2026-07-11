// Library Files
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "global.h"
#include "oled.h"
#include "ntp.h"
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
static char PopupText[32] = "";
static char PopupDetails[64] = "";
static uint32_t popupTimer = 0;
static uint8_t powerTimer = 10;
static uint32_t mainTimer = 0;
static uint32_t mainTimeout = OLED_MODE_NO_TIMEOUT;

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

void initDisplay() {
  display.init();
  oledMode = OLED_OFF;
  VextON();
}

void displayText() {
  static unsigned int cnt = 0;
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

  horOffset = (cnt / (60 * LOOPS_PER_SEC)) % 10;  // Every 60 seconds, span = 10 pixels  

  if (oledMode == OLED_MAIN) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Line 0: HEADER
    if (TEST_MODE) snprintf(line0, sizeof(line0), "SHAH LOG %s T", APP_VERSION);
    else           snprintf(line0, sizeof(line0), "SHAH LOG %s", APP_VERSION);
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
    float hours = cnt / 36000.0f;
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
      oledMode = OLED_MINIMIZED;  
    }
  }

  else if (oledMode == OLED_MINIMIZED) {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;

    uint32_t gallonsPerDay = (uint32_t)(5 * 60 * 24 / deltaMin);

    char cycMin[10];
    snprintf(cycMin, sizeof(cycMin), "%.1f", deltaMin);

    const unsigned int CYCLE_COUNT = 8;
    unsigned int mode = (cnt / MIN_MODE_CYCLE_TIME) % CYCLE_COUNT;

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
              (unsigned)(cnt / LOOPS_PER_SEC), 's',
              (unsigned)(cnt / 864000), 'd');
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

    if (cnt % (120 * LOOPS_PER_SEC) == 0) vertOffset = random(0, 50);

    display.display();
  }

  else if (oledMode == OLED_LOW_POWER) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Status Bar
    snprintf(line2, sizeof(line2), "LOW POWER in... %u", powerTimer);
    display.drawString(horOffset, 20 + vertOffset, line2);
    display.display();
    powerTimer--;
    if (powerTimer == 0) {
      lowPowerModeInit();
    }
  } 

  else if (oledMode == OLED_HALF_POWER) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Status Bar
    snprintf(line2, sizeof(line2), "HALF POWER in... %u", powerTimer);
    display.drawString(horOffset, 20 + vertOffset, line2);
    display.display();
    powerTimer--;
    if (powerTimer == 0) {
      oledMain();
    }
  } 

  cnt++;  // shared counter for all modes
}

void displayPopupScreen(const char* text, const char* details, uint16_t timeout, uint8_t next) {
  strncpy(PopupText, text, sizeof(PopupText) - 1);
  PopupText[sizeof(PopupText) - 1] = '\0';

  strncpy(PopupDetails, details, sizeof(PopupDetails) - 1);
  PopupDetails[sizeof(PopupDetails) - 1] = '\0';

  popupTimer = 0;
  oledMode = OLED_POPUP;
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

    popupTimer++;
  }
}

void newPopupScreen(const char* text, const char* details, uint16_t timeout, uint8_t next) {
  displayPopupScreen(text, details, timeout, next);
  updatePopupScreen();
}

void oledMain(uint32_t duration) {
  oledMode = OLED_MAIN;
  mainTimer = duration;
  mainTimeout = duration;
}

void oledMinimized() {
  oledMode = OLED_MINIMIZED;
}

void oledBlank() {
  oledMode = OLED_OFF;
}

void oledOff() {
  display.displayOff();
}

void oledOn() {
  display.displayOn();
}

void oledLowPower() {
  oledMode = OLED_LOW_POWER;
  powerTimer = OLED_POWER_TIMEOUT;
}

void oledHalfPower() {
  oledMode = OLED_HALF_POWER;
  powerTimer = OLED_POWER_TIMEOUT;
}


uint8_t getOledMode() {
  return oledMode;
}
