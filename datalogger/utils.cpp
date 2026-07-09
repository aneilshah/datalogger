// utils.cpp
#include "global.h"

uint32_t getCurrentEpoch();
static uint32_t s_bootMs = millis();

const char* getMonitorTime() {
  static char str[16];

  const float loopsPerSec = (float)LOOPS_PER_SEC;
  const float monMin = (float)LOOP_COUNT / (60.0f * loopsPerSec);
  const float monHr  = (float)LOOP_COUNT / (3600.0f * loopsPerSec);
  const float monDay = (float)LOOP_COUNT / (86400.0f * loopsPerSec);

  if (monHr < 1.0f) {
    snprintf(str, sizeof(str), "%.1f m", monMin);
  }
  else if (monDay < 1.0f) {
    snprintf(str, sizeof(str), "%.1f hr", monHr);
  }
  else {
    snprintf(str, sizeof(str), "%.1f day", monDay);
  }

  return str;
}

uint32_t msSinceBoot() {
  return (millis() - s_bootMs);
}

uint32_t minutesSinceBoot() {
  return msSinceBoot() / 60000UL;
}
uint32_t hoursSinceBoot() {
  return msSinceBoot() / 3600000UL;
}

//--------------------------------------------------
// Logging Functions
//--------------------------------------------------

void Log(String text) {
  Serial.println(text);
}

void VLog(String text) {
  if (VERBOSE) Serial.println(text);
}

void TLog(const char* msg)
// 1-arg overload: plain message
 {
  if (TEST_MODE) Serial.println(msg);
}

// 2-arg: timing only
void TLog(const char* label, uint32_t startTime) {
  if (!LOG_TIME) return;
  Serial.print(label);
  Serial.println(micros() - startTime);
}

// Date Helper
void getDayKeyForOffset(int daysAgo, char* out, size_t len) {
  time_t now = getCurrentEpoch();
  time_t t = now - (daysAgo * 86400);

  struct tm tm;
  localtime_r(&t, &tm);

  snprintf(out, len, "%04d_%02d_%02d",
    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

