#include "mode.h"
#include "logger.h"

static uint8_t loggerMode = MODE_INIT;
EventLogger Logger;
static uint32_t modeTimer = 0;

void setLoggerMode(uint8_t mode) {
  loggerMode = mode;
  modeTimer = 0;
}

uint8_t getLoggerMode() {return loggerMode;}
uint32_t getModeTimer()  {return modeTimer;}

void initLogger() {
  setLoggerMode(MODE_INIT);
  Logger.clear();
}

void processLoggerMode() {

  if (loggerMode == MODE_INIT) {

  }

  else if (loggerMode == MODE_LOGGING) {

  }

  else if (loggerMode == MODE_PAUSED) {

  }

  else if (loggerMode == MODE_CHECK_START) {

  }

  else if (loggerMode == MODE_CHECK_RESET) {

  }

  else {
    setLoggerMode(MODE_INIT);
  }

  modeTimer++;
}

