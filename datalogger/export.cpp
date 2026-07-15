// export.cpp

#include "export.h"

// Project Headers
#include "global.h"
#include "logger.h"
#include "loggerData.h"
#include "mode.h"
#include "ntp.h"
#include "nvm.h"
#include "ramlog.h"
#include "utils.h"

// External Data
extern uint16_t ADAPTIVE_DELAY;

// -----------------------------------------------------------------------------
// CSV Helpers
// -----------------------------------------------------------------------------
static void csvPrintQuoted(WiFiClient &client, const String &s) {
  client.print('"');
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '"') client.print('"');
    client.print(c);
  }
  client.print('"');
}


static void addTitle(WiFiClient &client, char* title) {
  client.println();
  client.println(title);
  client.println();
}

static void addTitle(WiFiClient &client, const __FlashStringHelper* title) {
  client.println();
  client.println(title);
  client.println();
}

// -----------------------------------------------------------------------------
// Export (CSV)
// -----------------------------------------------------------------------------


// Helpers

//*****************************************************************************
// Logger Flags to Text
//*****************************************************************************
const char *loggerFlagsToText(
  uint8_t flags,
  const LoggerFlagText *table,
  size_t count)
{
  static char text[64];

  text[0] = '\0';

  for (size_t i = 0; i < count; i++)
  {
    if (flags & table[i].flag)
    {
      if (text[0] != '\0')
        strcat(text, "|");

      strcat(text, table[i].text);
    }
  }

  if (text[0] == '\0')
    strcpy(text, "NONE");

  return text;
}

//*****************************************************************************
// Get Minute Flag Text
//*****************************************************************************
const char *getMinuteFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    minuteFlagTable,
    sizeof(minuteFlagTable) / sizeof(minuteFlagTable[0]));
}

//*****************************************************************************
// Get Hour Flag Text
//*****************************************************************************
const char *getHourFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    hourFlagTable,
    sizeof(hourFlagTable) / sizeof(hourFlagTable[0]));
}

//*****************************************************************************
// Get Session Flag Text
//*****************************************************************************
const char *getSessionFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    sessionFlagTable,
    sizeof(sessionFlagTable) / sizeof(sessionFlagTable[0]));
}

// -----------------------------------------------------------------------------
// MAIN DATA EXPORT
// -----------------------------------------------------------------------------
static void renderExportMainMetrics(WiFiClient &client)
{
  client.println(F("[LOGGER METRICS]"));

  const auto &session = Logger.getSessionStatistics();
  const auto &ramHeader  = Logger.getRamHeader();

  client.print(F("events,"));
  client.println(session.count);

  client.print(F("active_seconds,"));
  client.println(session.total);

  client.print(F("shortest_event_seconds,"));
  client.println(session.shortest);

  client.print(F("longest_event_seconds,"));
  client.println(session.longest);

  if (session.count > 0)
  {
    client.print(F("average_event_seconds,"));
    client.println((float)session.total / (float)session.count, 2);
  }

  client.print(F("hours_stored,"));
  client.println(ramHeader.hoursStored);

  client.print(F("samples_taken,"));
  client.println(ramHeader.samplesTaken);

  client.print(F("events_detected,"));
  client.println(Logger.hasEvents() ? F("1") : F("0"));

  client.print(F("session_start,"));
  client.println(ramHeader.startTime);

  client.print(F("session_stop,"));
  client.println(ramHeader.stopTime);

  client.print(F("timestamp,"));
  client.println(getTimestamp());
}

// -----------------------------------------------------------------------------
// Logger EXPORT
// -----------------------------------------------------------------------------
static void renderExportLoggerData(WiFiClient &client) {
  addTitle(client, F("[LOGGER]"));

}

// -----------------------------------------------------------------------------
// SYSTEM INFO EXPORT
// -----------------------------------------------------------------------------
static void renderExportSystemInfo(WiFiClient &client) {
  addTitle(client, F("[SYSTEM INFO]"));

  client.print(F("wifi_status,"));
  csvPrintQuoted(client, CONN_STATUS);
  client.println();

  client.print(F("wifi_err,"));
  client.println(WIFI_ERR);

  client.print(F("loop_time,"));
  client.println(LOOP_TIME);

  client.print(F("adaptive_delay_ms,"));
  client.println(ADAPTIVE_DELAY);

  client.print(F("timestamp,"));
  client.println(getTimestamp());
}

// -----------------------------------------------------------------------------
// Legger Data EXPORT
// -----------------------------------------------------------------------------
static void renderLoggerData(WiFiClient &client) {
  addTitle(client, F("[LOGGER DATA]"));

  // TODO: Summary Here

}

// -----------------------------------------------------------------------------
// CLOCK EXPORT
// -----------------------------------------------------------------------------
static void renderExportClockInfo(WiFiClient &client) {
  addTitle(client, F("[CLOCK INFO]"));

  char dayKey[16];
  getCurrentDayKey(dayKey, sizeof(dayKey));

  client.print(F("clock,"));
  csvPrintQuoted(client, getClock());
  client.println();

  client.print(F("date,"));
  csvPrintQuoted(client, getDate());
  client.println();

  client.print(F("timestamp,"));
  csvPrintQuoted(client, getTimestamp());
  client.println();

  client.print(F("epoch_time,"));
  client.println(getCurrentEpoch());

  client.print(F("clock_valid,"));
  client.println(validClock() ? F("1") : F("0"));

  client.print(F("current_day_key,"));
  csvPrintQuoted(client, dayKey);
  client.println();

  client.print(F("monitor_time,"));
  csvPrintQuoted(client, getMonitorTime());
  client.println();

  client.print(F("ms_since_boot,"));
  client.println(msSinceBoot());

  client.print(F("minutes_since_boot,"));
  client.println(minutesSinceBoot());

  client.print(F("hours_since_boot,"));
  client.println(hoursSinceBoot());

  client.print(F("hour_int,"));
  client.println(getHourInt());

  client.print(F("minute_int,"));
  client.println(getMinInt());

  client.print(F("second_int,"));
  client.println(getSecInt());

  client.print(F("day_int,"));
  client.println(getDayInt());

  client.print(F("month_int,"));
  client.println(getMonthInt());

  client.print(F("year_int,"));
  client.println(getYearInt());

  client.print(F("date_key_int,"));
  client.println(getDateKeyInt());
}


// -----------------------------------------------------------------------------
// NVM EXPORT
// -----------------------------------------------------------------------------
static void renderExportNvmInfo(WiFiClient &client) {
  addTitle(client, F("[NVM INFO]"));

  client.print(F("nvm_boot_count,"));
  client.println(nvmGetBootCount());

  client.print(F("nvm_last_boot_epoch,"));
  client.println(nvmGetLastBootEpoch());

  client.print(F("nvm_prev_boot_epoch,"));
  client.println(nvmGetPrevBootEpoch());
}

static void renderExportRamLog(WiFiClient& client) {
  addTitle(client, F("[RAM DEBUG LOG]"));

  client.println(F("idx,timestamp,message"));

  int start = (gLogIndex - gLogCount + LOG_LINES) % LOG_LINES;

  for (int i = 0; i < gLogCount; i++) {
    int idx = (start + i) % LOG_LINES;

    if (gLogMsg[idx][0] == '\0') continue;

    client.print(i);
    client.print(F(","));
    csvPrintQuoted(client, gLogTs[idx]);
    client.print(F(","));
    csvPrintQuoted(client, gLogMsg[idx]);
    client.println();
  }
}

// Logger Data CSV Export
static void renderExportLoggerDataCSV(WiFiClient& client)
{
  addTitle(client, F("[LOGGER DATA]"));

  client.println(
    F("timestamp,hour,minute,events,duration,shortest,longest,average,flag"));

  EventLogger::LogHeader header;

  if (!loggerDataReadNvmHeader(header))
    return;

  EventLogger::HourRecord hour;

  char timestamp[LOGGER_TIMESTAMP_LENGTH];

  for (uint16_t hourNumber = 0;
       hourNumber < header.hoursStored;
       hourNumber++)
  {
    if (!loggerDataReadHourBlock(hourNumber, hour))
      continue;

    for (uint8_t minute = 0; minute < 60; minute++)
    {
      const auto &m = hour.minute[minute];

      addMinutesToTimestamp(
        hour.startTime,
        minute,
        timestamp,
        sizeof(timestamp));

      float average = 0.0f;

      if (m.count > 0)
        average = (float)m.total / m.count;

      csvPrintQuoted(client, timestamp);
      client.print(",");

      client.print(hourNumber);
      client.print(",");

      client.print(minute);
      client.print(",");

      client.print(m.count);
      client.print(",");

      client.print(m.total);
      client.print(",");

      client.print(m.shortest);
      client.print(",");

      client.print(m.longest);
      client.print(",");

      client.print(average, 1);
      client.print(",");

      csvPrintQuoted(client, getMinuteFlagText(m.flags));

      client.println();
    }
  }
}


//------------------------------------------------------
// RENDER
//------------------------------------------------------

void renderExportCsv(WiFiClient &client) {
  // HTTP headers are emitted by the caller.
  // client.println(F("HTTP/1.1 200 OK"));
  // client.println(F("Content-Type: text/csv"));
  // client.println(F("Connection: close"));
  // client.println();

  client.print(F("# App Version,"));
  client.println(APP_VERSION);
  client.print(F("# Export Timestamp,"));
  client.println(getTimestamp());
  client.println();

  renderExportMainMetrics(client);
  renderExportLoggerData(client);
  renderExportSystemInfo(client);
  renderExportClockInfo(client);
  renderExportNvmInfo(client);
  renderExportLoggerData(client);
  renderExportRamLog(client);

}
