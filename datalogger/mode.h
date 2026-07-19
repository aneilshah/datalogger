#pragma once
#include "logger.h"

//-----------------------------------------------------
// Logger Modes
//-----------------------------------------------------

enum class LoggerMode : uint8_t
{
  RESET,
  LOGGING,
  PAUSED,
  STOPPED
};

enum class MenuScreen : uint8_t
{
  RESET,
  LOGGING,
  PAUSED,
  STOPPED,
  CHECK_START,
  CHECK_RESET,
  CHECK_RESET_AND_START,
  CHECK_RESTART,
  CHECK_PAUSE,
  CHECK_STOP
};

extern EventLogger Logger;

void processLoggerMode();
bool initLogger();
bool resetLogger();
bool restoreLoggerSession();
LoggerMode getLoggerMode();
MenuScreen getMenuScreen();
const char* getLoggerModeTxt();
const char* getMenuScreenTxt();
uint32_t getModeTimer();
uint32_t getScreenTimer();
