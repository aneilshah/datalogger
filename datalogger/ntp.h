#pragma once

void setupNTP();
void updateNTP();
bool validClock();
void updateDate();
void ensureTimeHealthy();

// Getters
const char* getClock();
const char* getDate();
const char* getTimestamp();
const char* getTimelog();
const char* getHourStr();
const char* getMinStr();
const char* getSecStr();
const char* getDayStr();
const char* getMonthStr();
const char* getYearStr();

uint8_t getHourInt();
uint8_t getMinInt();
uint8_t getSecInt();
uint8_t getDayInt();
uint8_t getMonthInt();
uint16_t getYearInt();
uint32_t getDateKeyInt();

// Utility
bool addMinutesToTimestamp(
  const char *timestamp,
  uint32_t minutes,
  char *result,
  size_t resultSize);

// FROM PUMP - MAYBE NOT NEEDED?
void getCurrentDayKey(char* buf, size_t len);
uint32_t getCurrentEpoch();