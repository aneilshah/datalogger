#pragma once
#include "Arduino.h"

#define LOG_LINES 100
#define LOG_LEN   50
#define TS_LEN    20

extern char gLogMsg[LOG_LINES][LOG_LEN];
extern char gLogTs[LOG_LINES][TS_LEN];
extern uint8_t gLogIndex;
extern uint8_t gLogCount;

void ramLog(const char* msg);
void ramLogf(const char* fmt, ...);