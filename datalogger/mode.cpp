#include "global.h"
#include "mode.h"

// Project Includes
#include "button.h"
#include "logger.h"
#include "loggerData.h"
#include "oled.h"
#include "power.h"
#include "ntp.h"
#include "nvm.h"


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


bool resetLogger()
{
  // Clear RAM
  Logger.clearRam();
  setLoggerMode(MODE_INIT);
  oledMain();

  // Erase logger data
  if (!loggerDataErase())
    return false;

  // Write fresh header
  if (!loggerDataWriteNvmHeader(Logger.getRamHeader()))
    return false;

  // Reset boot state
  if (!bootStateReset())
    return false;

  return true;
}

// Handle Transitions
// Set Mode, Power Mode, OLED, etc

void gotoInit() {
  resumeFullPowerMode();
  setLoggerMode(MODE_INIT);
  oledMain();
}

void gotoLogging() {
  lowPowerModeInit();
  setLoggerMode(MODE_LOGGING);
}

void gotoPaused() {
  resumeFullPowerMode();
  oledMain();
  setLoggerMode(MODE_PAUSED);
}

void gotoCheckReset() {


}

void gotoCheckStart() {
  setLoggerMode(MODE_CHECK_START);
  oledModal("START LOGGING?");
}

void gotoCheckRestart() {
  setLoggerMode(MODE_CHECK_RESTART);
  oledModal("RESTART LOGGING?");
}

void gotoCheckPause() {
  resumeHalfPowerMode();
  oledModal("PAUSE LOGGING?");
  setLoggerMode(MODE_CHECK_PAUSE);
}

bool initLogger()
{
  NvmBootState boot;

  Serial.println("Initializing Logger...");

  // Read boot state
  if (!bootStateRead(boot))
  {
    Serial.println("No valid boot state. Resetting logger.");
    resetLogger();
    return false;
  }

  // Read logger header
  EventLogger::LogHeader header;

  if (!loggerDataReadNvmHeader(header))
  {
    Serial.println("Invalid logger header. Resetting logger.");
    resetLogger();
    return false;
  }

  Logger.setRamHeader(header);

  // Validate header
  if (Logger.getRamHeader().magic != LOGGER_MAGIC)
  {
    Serial.println("Invalid logger magic. Resetting logger.");
    resetLogger();
    return false;
  }

  if (Logger.getRamHeader().version != LOGGER_VERSION)
  {
    Serial.println("Logger version mismatch. Resetting logger.");
    resetLogger();
    return false;
  }

  if (boot.hoursStored != Logger.getRamHeader().hoursStored)
  {
    Serial.println("Hour count mismatch - RAM vs NVM.");
  }

  if (!boot.sessionActive)
  {
    Serial.println("No active session.");
    setLoggerMode(MODE_INIT);
    return false;
  }

  // Session was ACTIVE
  boot.sessionFlags |= SESSION_FLAG_REBOOT;

  // Check for PAUSED
  if (boot.sessionPaused) {  
    gotoPaused();
  }

  // else was LOGGING
  else {
    gotoLogging();
  }

  if (!bootStateWrite(boot))
  {
    Serial.println("Failed to update boot state.");
    return false;
  }

  Serial.println("Recovered active session.");
  return true;
}

void processLoggerMode() {

  // Init Mode, full power not running
  if (loggerMode == MODE_INIT) {
    if (shortPress()) {
      gotoCheckStart();
    }
  }

  // Logging Mode
  else if (loggerMode == MODE_LOGGING) {
    if (shortPress()) {
      gotoCheckPause();
    }
  }

  // Paused, has data
  else if (loggerMode == MODE_PAUSED) {
    if (shortPress()) {
      gotoCheckRestart();
    }
  }

  // Checking to start logging from init
  else if (loggerMode == MODE_CHECK_START) {
    if (modalEvent()) {
      gotoLogging();
      Logger.startNewSession();
    }
    else if (shortPress()) {
      gotoInit();
    }
  }

  // Check if user wants to Reset Logger
  else if (loggerMode == MODE_CHECK_RESET) {
    if (modalEvent()) {
      gotoInit();
    }
    else if (shortPress()) {
      gotoPaused();
    }
  }

  // Check if user wants to Pause Logging
  else if (loggerMode == MODE_CHECK_PAUSE) {
    if (modalEvent()) {
      gotoPaused();
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