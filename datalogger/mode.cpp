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
static LoggerMode loggerMode = LoggerMode::RESET;
static MenuScreen menuScreen = MenuScreen::RESET;
EventLogger Logger;
static uint32_t modeTimer = 0;
static uint32_t screenTimer = 0;

// Methods

const char* getLoggerModeTxt() {
  if (loggerMode == LoggerMode::RESET) return "RESET";
  if (loggerMode == LoggerMode::LOGGING) return "LOGGING";
  if (loggerMode == LoggerMode::PAUSED) return "PAUSED";
  if (loggerMode == LoggerMode::STOPPED) return "STOPPED";
  else return "UNKNOWN_MODE";
}

const char* getMenuScreenTxt() {
  if (menuScreen == MenuScreen::RESET) return "RESET";
  if (menuScreen == MenuScreen::LOGGING) return "LOGGING";
  if (menuScreen == MenuScreen::PAUSED) return "PAUSED";
  if (menuScreen == MenuScreen::STOPPED) return "STOPPED";
  if (menuScreen == MenuScreen::CHECK_START) return "CHECK_START";
  if (menuScreen == MenuScreen::CHECK_PAUSE) return "CHECK_PAUSE";
  if (menuScreen == MenuScreen::CHECK_RESTART) return "CHECK_RESTART";
  if (menuScreen == MenuScreen::CHECK_STOP) return "CHECK_STOP";
  if (menuScreen == MenuScreen::CHECK_RESET) return "CHECK_RESET";
  if (menuScreen == MenuScreen::CHECK_RESET_AND_START) return "CHECK_RESET_AND_START";
  else return "UNKNOWN_SCREEN";

}

void setLoggerMode(LoggerMode mode) {
  loggerMode = mode;
  modeTimer = 0;
  setBootLoggerMode(mode);
  Serial.printf("Logger Mode: %s\n", getLoggerModeTxt());
}

void setMenuScreen(MenuScreen screen) {
  menuScreen = screen;
  screenTimer = 0;
  clearModalEvent();
  Serial.printf("Menu Screen: %s\n", getMenuScreenTxt());
}

LoggerMode getLoggerMode() {return loggerMode;}
MenuScreen getMenuScreen() {return menuScreen;}
uint32_t getModeTimer()  {return modeTimer;}
uint32_t getScreenTimer()  {return screenTimer;}


bool resetLogger()
{
  // Clear RAM and Sessions
  Logger.clearRam();
  Logger.clearHour();
  Logger.clearMinuteStats();


  setLoggerMode(LoggerMode::RESET);
  setMenuScreen(MenuScreen::RESET);
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
  setLoggerMode(LoggerMode::RESET);
  setMenuScreen(MenuScreen::RESET);
  oledMain();
}

void gotoLogging() {
  lowPowerModeInit();
  setLoggerMode(LoggerMode::LOGGING);
  setMenuScreen(MenuScreen::LOGGING);
  setBootLoggerMode(LoggerMode::LOGGING);
}

void gotoPaused() {
  resumeFullPowerMode();
  oledMain();
  setLoggerMode(LoggerMode::PAUSED);
  setMenuScreen(MenuScreen::PAUSED);
  checkpointNvm();
  loggerDataWriteNvmHeader(Logger.getRamHeader()); // NVM
}

void gotoStopped() {
  resumeFullPowerMode();
  oledMain();
  setLoggerMode(LoggerMode::STOPPED);
  setMenuScreen(MenuScreen::STOPPED);
  checkpointNvm();
  Logger.stopSession();  
}

void gotoCheckReset() {
  setMenuScreen(MenuScreen::CHECK_RESET);
  oledModal("!! RESET DATA?");
}

void gotoCheckResetAndStart() {
  setMenuScreen(MenuScreen::CHECK_RESET_AND_START);
  oledModal("!! RESET AND RESTART?");
}

void gotoCheckStart() {
  setMenuScreen(MenuScreen::CHECK_START);
  oledModal("START LOGGING?");
}

void gotoCheckRestart() {
  setMenuScreen(MenuScreen::CHECK_RESTART);
  oledModal("RESTART LOGGING?");
}

void gotoCheckStop() {
  setMenuScreen(MenuScreen::CHECK_STOP);
  oledModal("STOP LOGGING?");
}

void gotoCheckPause() {
  resumeHalfPowerMode();
  oledModal("PAUSE LOGGING?");
  setMenuScreen(MenuScreen::CHECK_PAUSE);
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
  EventLogger::LogHeader nvmHeader;

  if (!loggerDataReadNvmHeader(nvmHeader))
  {
    Serial.println("Invalid logger NVM header. Resetting logger.");
    resetLogger();
    return false;
  }

  Logger.setRamHeader(nvmHeader);

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

  LoggerMode mode = boot.loggerMode;

  switch (mode)
  {
    case LoggerMode::PAUSED:
      gotoPaused();
      break;

    case LoggerMode::STOPPED:
      gotoStopped();
      break;

    case LoggerMode::LOGGING:
      gotoLogging();
      break;

    case LoggerMode::RESET:
    default:
      gotoInit();
      break;
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
  if (menuScreen == MenuScreen::RESET) {
    if (shortPress()) {
      gotoCheckStart();
    }
  }

  // Logging Mode
  else if (menuScreen == MenuScreen::LOGGING) {
    if (shortPress()) {
      gotoCheckPause();
    }
  }

  // Paused, has data
  else if (menuScreen == MenuScreen::PAUSED) {
    if (shortPress()) {
      gotoCheckRestart();
    }
  }

  // Stopped, has data
  else if (menuScreen == MenuScreen::STOPPED) {
    if (shortPress()) {
      gotoCheckReset();
    }
  }

  // Checking to start logging from init
  else if (menuScreen == MenuScreen::CHECK_START) {
    if (modalEvent()) {
      gotoLogging();
      Logger.startNewSession();
    }
    else if (shortPress()) {
      gotoInit();
    }
  }

  // Check if user wants to Reset Logger
  else if (menuScreen == MenuScreen::CHECK_RESET) {
    if (modalEvent()) {
      resetLogger();
    }
    else if (shortPress()) {
      gotoCheckResetAndStart();
    }
  }

  // Check if user wants to Reset Logger and Restart
  else if (menuScreen == MenuScreen::CHECK_RESET_AND_START) {
    if (modalEvent()) {
      resetLogger();
      gotoLogging();
      Logger.startNewSession();
    }
    else if (shortPress()) {
      gotoStopped();
    }
  }

  // Check if user wants to Pause Logging
  else if (menuScreen == MenuScreen::CHECK_PAUSE) {
    if (modalEvent()) {
      gotoPaused();
    }
    else if (shortPress()) {
      gotoLogging();
      loggerDataWriteNvmHeader(Logger.getRamHeader()); // NVM
    }
  }

  // Check if user wants to Restart from Paused state
  else if (menuScreen == MenuScreen::CHECK_RESTART) {
    if (modalEvent()) {
      lowPowerModeInit();
      gotoLogging();
    }
    else if (shortPress()) {
      gotoCheckStop();
    }
  }

  // Check if user wants to Restart from Paused state
  else if (menuScreen == MenuScreen::CHECK_STOP) {
    if (modalEvent()) {
      gotoStopped();
    }
    else if (shortPress()) {
      gotoPaused();
    }
  }

  // Default to Init Mode (Reset), should not get here
  else {
    resetLogger();
  }

  modeTimer++;
  screenTimer++;
}