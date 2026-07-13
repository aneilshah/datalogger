#include "global.h"
#include "mode.h"

// Project Includes
#include "button.h"
#include "logger.h"
#include "oled.h"
#include "power.h"


// File Data
static uint8_t loggerMode = MODE_INIT;
EventLogger Logger;
static uint32_t modeTimer = 0;

// Methods

const char* getLoggerModeTxt() {
  if (loggerMode == MODE_INIT) return "INIT";
  if (loggerMode == MODE_LOGGING) return "LOGGING";
  if (loggerMode == MODE_PAUSED) return "PAUSED";
  if (loggerMode == MODE_CHECK_START) return "CHECK_START";
  if (loggerMode == MODE_CHECK_RESET) return "CHECK_RESET";
  if (loggerMode == MODE_CHECK_PAUSE) return "CHECK_PAUSE";
  if (loggerMode == MODE_CHECK_RESTART) return "CHECK_RESTART";
  else return "UNKNOWN";

}

void setLoggerMode(uint8_t mode) {
  loggerMode = mode;
  modeTimer = 0;
  clearModalEvent();
  Serial.printf("Logger Mode: %s\n", getLoggerModeTxt());
}

uint8_t getLoggerMode() {return loggerMode;}
uint32_t getModeTimer()  {return modeTimer;}

void initLogger() {
  setLoggerMode(MODE_INIT);
  Logger.clear();
}

void processLoggerMode() {

  // Init Mode, full power not running
  if (loggerMode == MODE_INIT) {
    if (shortPress()) {
      setLoggerMode(MODE_CHECK_START);
      oledModal("START LOGGING?");
    }
  }

  // Logging Mode
  else if (loggerMode == MODE_LOGGING) {
    if (shortPress()) {
      setLoggerMode(MODE_CHECK_PAUSE);
      resumeHalfPowerMode();
      oledModal("PAUSE LOGGING?");
    }
  }

  // Paused, has data
  else if (loggerMode == MODE_PAUSED) {
    if (shortPress()) {
      setLoggerMode(MODE_CHECK_RESTART);
      oledModal("RESTART LOGGING?");
    }
  }

  // Checking to start logging from init
  else if (loggerMode == MODE_CHECK_START) {
    if (modalEvent()) {
      lowPowerModeInit();
      setLoggerMode(MODE_LOGGING);
    }
    else if (shortPress()) {
      setLoggerMode(MODE_INIT);
    }
  }

  // Check if user wants to Reset Logger
  else if (loggerMode == MODE_CHECK_RESET) {
    if (modalEvent()) {
      lowPowerModeInit();
      // TODO: ADD RESET CODE HERE (logger.reset() etc)
      setLoggerMode(MODE_INIT);
    }
    else if (shortPress()) {
      setLoggerMode(MODE_PAUSED);
      oledMain();
    }
  }

  // Check if user wants to Pause Logging
  else if (loggerMode == MODE_CHECK_PAUSE) {
    if (modalEvent()) {
      resumeFullPowerMode();
      //TODO: Might need new OLED screen here?
      Serial.println("OLED MAIN");
      oledMain();
      setLoggerMode(MODE_PAUSED);
    }
    else if (shortPress()) {
      setLoggerMode(MODE_LOGGING);
      oledOff();
    }
  }

  // Check if user wants to Restart from Paused state
  else if (loggerMode == MODE_CHECK_RESTART) {
    if (modalEvent()) {
      lowPowerModeInit();
      setLoggerMode(MODE_LOGGING);
    }
    else if (shortPress()) {
      setLoggerMode(MODE_CHECK_RESET);
      oledModal("RESET DATA?");
    }
  }

  // Default to Init Mode, should not get here
  else {
    setLoggerMode(MODE_INIT);
    // Todo: reset logger?  Go to Error Banner?
  }

  modeTimer++;
}