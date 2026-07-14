#pragma once
#include "logger.h"

//-----------------------------------------------------
// Logger Modes
//-----------------------------------------------------

#define MODE_INIT     0
#define MODE_LOGGING  1
#define MODE_PAUSED   2
#define MODE_CHECK_START 3
#define MODE_CHECK_RESET 4
#define MODE_CHECK_PAUSE 5
#define MODE_CHECK_RESTART 6

extern EventLogger Logger;

void processLoggerMode();
void initLogger();
bool resetLogger();
uint8_t getLoggerMode();
uint32_t getModeTimer();