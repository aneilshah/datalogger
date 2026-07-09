// utils.h
#pragma once
#include "global.h"
#include <Arduino.h>

// Time and Date
const char* getMonitorTime();
uint32_t msSinceBoot();
uint32_t hoursSinceBoot();
uint32_t minutesSinceBoot();
void getDayKeyForOffset(int daysAgo, char* out, size_t len);

// Logging
void Log(String text);
void VLog(String text);
void TLog(const char* msg);
void TLog(const char* label, uint32_t startTime);

// 2-arg overload: label + value (template)
template<typename T>
void TLog(const char* label, const T& value) {
  if (!TEST_MODE) return;
  Serial.print(label);
  Serial.println(value);
}

// 3-arg: label + value + timing
template<typename T>
void TLog(const char* label, const T& value, uint32_t startTime) {
  if (!LOG_TIME) return;
  Serial.print(label);
  Serial.print(value);
  Serial.print(" ");
  Serial.println(micros() - startTime);
}
