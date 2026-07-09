#pragma once
#include <Arduino.h>
#include <string.h>

struct PumpDailyRecord {
  int cyclesPerDay;
  int gallonsPerDay;
};

class Pump365Data {
public:
  static const int DAILY_DAYS = 365;

  void begin() {
    clearDailyHistory();
  }

  void clearDailyHistory() {
    memset(_dailyCyclesPerDay, 0, sizeof(_dailyCyclesPerDay));
    memset(_dailyGallonsPerDay, 0, sizeof(_dailyGallonsPerDay));
    _dailyHead = -1;
    _dailyValidCount = 0;
  }

  int dailyValidCount() const {
    return _dailyValidCount;
  }

  void recordDailySummary(int cyclesPerDay, int gallonsPerDay) {
    _dailyHead = (_dailyHead + 1) % DAILY_DAYS;
    _dailyCyclesPerDay[_dailyHead]  = sanitizeInt(cyclesPerDay);
    _dailyGallonsPerDay[_dailyHead] = sanitizeInt(gallonsPerDay);

    if (_dailyValidCount < DAILY_DAYS) {
      _dailyValidCount++;
    }
  }

  bool getDailyRecordAgo(int daysAgo, PumpDailyRecord& out) const {
    if (_dailyHead < 0) return false;
    if (daysAgo < 0 || daysAgo >= _dailyValidCount) return false;

    const int idx = dailyIndexFromDaysAgo(daysAgo);
    out.cyclesPerDay  = _dailyCyclesPerDay[idx];
    out.gallonsPerDay = _dailyGallonsPerDay[idx];
    return true;
  }

  bool getDailyRecordAgo(int daysAgo, int& outCycles, int& outGallons) const {
    if (_dailyHead < 0) return false;
    if (daysAgo < 0 || daysAgo >= _dailyValidCount) return false;

    const int idx = dailyIndexFromDaysAgo(daysAgo);
    outCycles  = _dailyCyclesPerDay[idx];
    outGallons = _dailyGallonsPerDay[idx];
    return true;
  }

  bool appendOlderDailyRecord(int cyclesPerDay, int gallonsPerDay) {
    if (_dailyValidCount >= DAILY_DAYS) return false;

    if (_dailyHead < 0) {
      _dailyHead = 0;
      _dailyCyclesPerDay[_dailyHead]  = sanitizeInt(cyclesPerDay);
      _dailyGallonsPerDay[_dailyHead] = sanitizeInt(gallonsPerDay);
      _dailyValidCount = 1;
      return true;
    }

    int oldestValidIdx = (_dailyHead - (_dailyValidCount - 1)) % DAILY_DAYS;
    if (oldestValidIdx < 0) oldestValidIdx += DAILY_DAYS;

    int newOldestIdx = (oldestValidIdx - 1) % DAILY_DAYS;
    if (newOldestIdx < 0) newOldestIdx += DAILY_DAYS;

    _dailyCyclesPerDay[newOldestIdx]  = sanitizeInt(cyclesPerDay);
    _dailyGallonsPerDay[newOldestIdx] = sanitizeInt(gallonsPerDay);
    _dailyValidCount++;
    return true;
  }

  bool appendOlderMissingDailyRecord() {
    return appendOlderDailyRecord(-1, -1);
  }

  bool getDailySample(int offsetOldestDay, PumpDailyRecord& out) const {
    if (_dailyHead < 0) return false;
    if (offsetOldestDay < 0 || offsetOldestDay >= _dailyValidCount) return false;

    int oldestValidIdx = (_dailyHead - (_dailyValidCount - 1)) % DAILY_DAYS;
    if (oldestValidIdx < 0) oldestValidIdx += DAILY_DAYS;

    int idx = (oldestValidIdx + offsetOldestDay) % DAILY_DAYS;

    out.cyclesPerDay  = _dailyCyclesPerDay[idx];
    out.gallonsPerDay = _dailyGallonsPerDay[idx];
    return true;
  }

private:
  int   _dailyCyclesPerDay[DAILY_DAYS];
  int   _dailyGallonsPerDay[DAILY_DAYS];

  int _dailyHead = -1;
  int _dailyValidCount = 0;

  static int sanitizeInt(int v) {
    return (v < 0) ? -1 : v;
  }

  int dailyIndexFromDaysAgo(int daysAgo) const {
    int idx = (_dailyHead - daysAgo) % DAILY_DAYS;
    if (idx < 0) idx += DAILY_DAYS;
    return idx;
  }
};
