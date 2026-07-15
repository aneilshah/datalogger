#include <time.h>
#include <sys/time.h>

// Project Files
#include "global.h"
#include "diag.h"
#include "ntp.h"

//--------------------------------------------------------
// NTP / Local Time Variables
//--------------------------------------------------------

unsigned int Month = 0;
unsigned int Day = 0;
unsigned int Year = 0;

// US Eastern Time with automatic DST
// Standard: EST (UTC-5)
// Daylight: EDT (UTC-4)
// DST starts: 2nd Sunday in March at 2:00
// DST ends:   1st Sunday in November at 2:00
static const char* TZ_INFO = "EST5EDT,M3.2.0/2,M11.1.0/2";

//--------------------------------------------------------
// Internal Helpers
//--------------------------------------------------------

static bool getLocalTm(struct tm* outTm) {
  if (!outTm) return false;

  time_t now = time(nullptr);
  if (now <= 0) return false;

  localtime_r(&now, outTm);
  return true;
}

static bool getGmTm(struct tm* outTm) {
  if (!outTm) return false;

  time_t now = time(nullptr);
  if (now <= 0) return false;

  gmtime_r(&now, outTm);
  return true;
}

//--------------------------------------------------------
// NTP Functions
//--------------------------------------------------------

void setupNTP() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", TZ_INFO, 1);
  tzset();
}

void updateNTP() {
  // With configTzTime(), SNTP refresh happens in the background.
  // We just refresh cached date fields when time is valid.
  if (validClock()) {
    updateDate();
  }
}

const char* getClock() {
  static char buf[9];  // "HH:MM:SS" + null
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(buf, sizeof(buf), "--:--:--");
    return buf;
  }

  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
           tmv.tm_hour,
           tmv.tm_min,
           tmv.tm_sec);
  return buf;
}

void updateDate() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return;

  Month = (unsigned int)(tmv.tm_mon + 1);
  Day   = (unsigned int)tmv.tm_mday;
  Year  = (unsigned int)(tmv.tm_year + 1900);
}

const char* getDate() {
  static char ts[16];
  snprintf(ts, sizeof(ts), "%s.%s.%s",
           getYearStr(), getMonthStr(), getDayStr());
  return ts;
}

const char* getTimestamp() {
  // Format 2026.03.16T18:47:57
  static char ts[24];
  snprintf(ts, sizeof(ts), "%s.%s.%sT%s:%s:%s",
           getYearStr(), getMonthStr(), getDayStr(),
           getHourStr(), getMinStr(), getSecStr());
  return ts;
}

const char* getTimelog() {
  // Format 2026_03_16T18_47_57
  static char ts[24];
  snprintf(ts, sizeof(ts), "%s_%s_%sT%s_%s_%s",
           getYearStr(), getMonthStr(), getDayStr(),
           getHourStr(), getMinStr(), getSecStr());
  return ts;
}

const char* getHourStr() {
  static char hh[3];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(hh, sizeof(hh), "00");
    return hh;
  }

  snprintf(hh, sizeof(hh), "%02d", tmv.tm_hour);
  return hh;
}

const char* getMinStr() {
  static char mm[3];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(mm, sizeof(mm), "00");
    return mm;
  }

  snprintf(mm, sizeof(mm), "%02d", tmv.tm_min);
  return mm;
}

const char* getSecStr() {
  static char ss[3];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(ss, sizeof(ss), "00");
    return ss;
  }

  snprintf(ss, sizeof(ss), "%02d", tmv.tm_sec);
  return ss;
}

const char* getYearStr() {
  static char yyyy[6];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(yyyy, sizeof(yyyy), "0000");
    return yyyy;
  }

  snprintf(yyyy, sizeof(yyyy), "%04d", tmv.tm_year + 1900);
  return yyyy;
}

const char* getMonthStr() {
  static char mm[3];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(mm, sizeof(mm), "00");
    return mm;
  }

  snprintf(mm, sizeof(mm), "%02d", tmv.tm_mon + 1);
  return mm;
}

const char* getDayStr() {
  static char dd[3];
  struct tm tmv;

  if (!getLocalTm(&tmv)) {
    snprintf(dd, sizeof(dd), "00");
    return dd;
  }

  snprintf(dd, sizeof(dd), "%02d", tmv.tm_mday);
  return dd;
}

uint8_t getHourInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint8_t)tmv.tm_hour;
}

uint8_t getMinInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint8_t)tmv.tm_min;
}

uint8_t getSecInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint8_t)tmv.tm_sec;
}

uint8_t getDayInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint8_t)tmv.tm_mday;
}

uint8_t getMonthInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint8_t)(tmv.tm_mon + 1);
}

uint16_t getYearInt() {
  struct tm tmv;
  if (!getLocalTm(&tmv)) return 0;
  return (uint16_t)(tmv.tm_year + 1900);
}

// NOTE: getDateKeyInt currently returns MMDD (no year).
// Safe for current system due to daily reset + validity filtering.
// Can be upgraded to YYYYMMDD in future without changing callers.
uint32_t getDateKeyInt() {
  return 100*getMonthInt() + getDayInt();
}

bool validClock() {
  time_t now = time(nullptr);
  return (now > 1767225600 && now < 2556144000); // 2026 - 2050
}

// FROM PUMP - MAYBE NOT NEEDED?

void getCurrentDayKey(char* buf, size_t len) {
  snprintf(buf, len, "%s_%s_%s",
           getYearStr(),
           getMonthStr(),
           getDayStr());
}

uint32_t getCurrentEpoch() {
  time_t now = time(nullptr);
  if (now <= 1000000000) {  // crude "time not set" guard (~2001)
    return 0;
  }
  return (uint32_t)now;
}

//-----------------------------------------------
// Validation Helpers
//-----------------------------------------------

bool isTimezoneApplied() {
  time_t now = time(nullptr);

  // If time isn't valid yet, don't evaluate
  if (now <= 1000000000) return true;

  struct tm localTm, gmTm;
  localtime_r(&now, &localTm);
  gmtime_r(&now, &gmTm);

  int diffHours = localTm.tm_hour - gmTm.tm_hour;

  // Normalize to [-12, +12]
  if (diffHours > 12) diffHours -= 24;
  if (diffHours < -12) diffHours += 24;

  // Eastern should be:
  // -5 hours (EST) or -4 hours (EDT)
  return (diffHours == -5 || diffHours == -4);
}

void ensureTimeHealthy() {
  if (!validClock()) return;

  if (!isTimezoneApplied()) {
    Serial.println("⚠️ Timezone not applied — fixing");

    setenv("TZ", TZ_INFO, 1);
    tzset();
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  }
}

//*****************************************************************************
// Add Minutes to Timestamp
//*****************************************************************************
bool addMinutesToTimestamp(
  const char *timestamp,
  uint32_t minutes,
  char *result,
  size_t resultSize)
{
  struct tm tm = {};

  if (sscanf(timestamp,
    "%4d.%2d.%2dT%2d:%2d:%2d",
    &tm.tm_year,
    &tm.tm_mon,
    &tm.tm_mday,
    &tm.tm_hour,
    &tm.tm_min,
    &tm.tm_sec) != 6)
  {
    return false;
  }

  tm.tm_year -= 1900;
  tm.tm_mon--;

  time_t t = mktime(&tm);

  if (t == (time_t)-1)
    return false;

  t += (time_t)minutes * 60;

  struct tm *newTm = localtime(&t);

  if (newTm == nullptr)
    return false;

  snprintf(result,
    resultSize,
    "%04d.%02d.%02dT%02d:%02d:%02d",
    newTm->tm_year + 1900,
    newTm->tm_mon + 1,
    newTm->tm_mday,
    newTm->tm_hour,
    newTm->tm_min,
    newTm->tm_sec);

  return true;
}