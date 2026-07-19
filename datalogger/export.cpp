// export.cpp

#include "export.h"

// Project Headers
#include "global.h"
#include "bufferedPrint.h"
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
static void csvPrintQuoted(Print &out, const String &s) {
  out.print('"');
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '"') out.print('"');
    out.print(c);
  }
  out.print('"');
}


static void addTitle(Print &out, char* title) {
  out.println();
  out.println(title);
  out.println();
}

static void addTitle(Print &out, const __FlashStringHelper* title) {
  out.println();
  out.println(title);
  out.println();
}

// -----------------------------------------------------------------------------
// Export (CSV)
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MAIN DATA EXPORT
// -----------------------------------------------------------------------------
static void renderExportMainMetrics(Print &out)
{
  out.println(F("[LOGGER METRICS]"));

  const auto &session = Logger.getSessionStatistics();
  const auto &ramHeader  = Logger.getRamHeader();

  out.print(F("events,"));
  out.println(session.count);

  out.print(F("active_seconds,"));
  out.println(session.total);

  out.print(F("shortest_event_seconds,"));
  out.println(session.shortest);

  out.print(F("longest_event_seconds,"));
  out.println(session.longest);

  if (session.count > 0)
  {
    out.print(F("average_event_seconds,"));
    out.println((float)session.total / (float)session.count, 2);
  }

  out.print(F("hours_stored,"));
  out.println(ramHeader.hoursStored);

  out.print(F("samples_taken,"));
  out.println(ramHeader.samplesTaken);

  out.print(F("events_detected,"));
  out.println(Logger.hasEvents() ? F("1") : F("0"));

  out.print(F("session_start,"));
  out.println(ramHeader.startTime);

  out.print(F("session_stop,"));
  out.println(ramHeader.stopTime);

  out.print(F("timestamp,"));
  out.println(getTimestamp());
}

// -----------------------------------------------------------------------------
// Logger EXPORT
// -----------------------------------------------------------------------------
static void renderExportLoggerData(Print &out) {
  addTitle(out, F("[LOGGER]"));

}

// -----------------------------------------------------------------------------
// SYSTEM INFO EXPORT
// -----------------------------------------------------------------------------
static void renderExportSystemInfo(Print &out) {
  addTitle(out, F("[SYSTEM INFO]"));

  out.print(F("wifi_status,"));
  csvPrintQuoted(out, CONN_STATUS);
  out.println();

  out.print(F("wifi_err,"));
  out.println(WIFI_ERR);

  out.print(F("loop_time,"));
  out.println(LOOP_TIME);

  out.print(F("adaptive_delay_ms,"));
  out.println(ADAPTIVE_DELAY);

  out.print(F("timestamp,"));
  out.println(getTimestamp());
}

// -----------------------------------------------------------------------------
// Legger Data EXPORT
// -----------------------------------------------------------------------------
static void renderLoggerData(Print &out) {
  addTitle(out, F("[LOGGER DATA]"));

  // TODO: Summary Here

}

// -----------------------------------------------------------------------------
// CLOCK EXPORT
// -----------------------------------------------------------------------------
static void renderExportClockInfo(Print &out) {
  addTitle(out, F("[CLOCK INFO]"));

  char dayKey[16];
  getCurrentDayKey(dayKey, sizeof(dayKey));

  out.print(F("clock,"));
  csvPrintQuoted(out, getClock());
  out.println();

  out.print(F("date,"));
  csvPrintQuoted(out, getDate());
  out.println();

  out.print(F("timestamp,"));
  csvPrintQuoted(out, getTimestamp());
  out.println();

  out.print(F("epoch_time,"));
  out.println(getCurrentEpoch());

  out.print(F("clock_valid,"));
  out.println(validClock() ? F("1") : F("0"));

  out.print(F("current_day_key,"));
  csvPrintQuoted(out, dayKey);
  out.println();

  out.print(F("monitor_time,"));
  csvPrintQuoted(out, getMonitorTime());
  out.println();

  out.print(F("ms_since_boot,"));
  out.println(msSinceBoot());

  out.print(F("minutes_since_boot,"));
  out.println(minutesSinceBoot());

  out.print(F("hours_since_boot,"));
  out.println(hoursSinceBoot());

  out.print(F("hour_int,"));
  out.println(getHourInt());

  out.print(F("minute_int,"));
  out.println(getMinInt());

  out.print(F("second_int,"));
  out.println(getSecInt());

  out.print(F("day_int,"));
  out.println(getDayInt());

  out.print(F("month_int,"));
  out.println(getMonthInt());

  out.print(F("year_int,"));
  out.println(getYearInt());

  out.print(F("date_key_int,"));
  out.println(getDateKeyInt());
}


// -----------------------------------------------------------------------------
// NVM EXPORT
// -----------------------------------------------------------------------------
static void renderExportNvmInfo(Print &out)
{
  addTitle(out, F("[NVM BOOT INFO]"));

  out.print(F("nvm_boot_count,"));
  out.println(nvmGetBootCount());

  NvmBootState boot;

  if (!bootStateRead(boot))
  {
    out.println(F("boot_state,Not Found"));
    return;
  }

  out.print(F("boot_logger_mode,0x"));
  uint8_t mode = static_cast<uint8_t>(boot.loggerMode);
  if (mode < 0x10) out.print('0');
  out.print(mode, HEX);
  out.print(F(" ("));
  out.print(getBootLoggerModeTxt());
  out.println(')');

  out.print(F("hours_stored,"));
  out.println(boot.hoursStored);

  out.print(F("last_save,"));
  out.println(boot.saveTimestamp);

  out.println();
}

static void renderExportRamLog(Print &out) {
  addTitle(out, F("[RAM DEBUG LOG]"));

  out.println(F("idx,timestamp,message"));

  int start = (gLogIndex - gLogCount + LOG_LINES) % LOG_LINES;

  for (int i = 0; i < gLogCount; i++) {
    int idx = (start + i) % LOG_LINES;

    if (gLogMsg[idx][0] == '\0') continue;

    out.print(i);
    out.print(F(","));
    csvPrintQuoted(out, gLogTs[idx]);
    out.print(F(","));
    csvPrintQuoted(out, gLogMsg[idx]);
    out.println();
  }
}

void renderExportLoggerDataCsv(Print &out)
{
  //*****************************************************************************
  // Session Summary
  //*****************************************************************************
  const auto &session = Logger.getSessionStatistics();
  const auto &ramHeader = Logger.getRamHeader();

  out.print(F("# App Version,"));
  out.println(APP_VERSION);

  out.print(F("# Export Timestamp,"));
  out.println(getTimestamp());

  out.println();

  out.println(F("# Session"));

  out.print(F("# Start Time,"));
  out.println(ramHeader.startTime);

  out.print(F("# Stop Time,"));
  out.println(ramHeader.stopTime);

  out.print(F("# Hours Stored,"));
  out.println(ramHeader.hoursStored);

  out.print(F("# Events,"));
  out.println(session.count);

  out.print(F("# Active Seconds,"));
  out.println(session.total);

  if (session.count > 0)
  {
    out.print(F("# Shortest Event,"));
    out.println(session.shortest);

    out.print(F("# Longest Event,"));
    out.println(session.longest);

    out.print(F("# Average Event,"));
    out.println((float)session.total / session.count, 1);
  }
  else
  {
    out.println(F("# Shortest Event,0"));
    out.println(F("# Longest Event,0"));
    out.println(F("# Average Event,0.0"));
  }

  out.print(F("# Session Flags,"));
  out.println(getSessionFlagText(ramHeader.flags));

  out.println();

  //*****************************************************************************
  // Data
  //*****************************************************************************

  out.println(
    F("timestamp,hour,minute,events,duration,shortest,longest,average,flag"));

  EventLogger::LogHeader nvmHeader;

  if (!loggerDataReadNvmHeader(nvmHeader))
  {
    out.flush();
    return;
  }

  EventLogger::HourRecord hour;

  char timestamp[LOGGER_TIMESTAMP_LENGTH];

  for (uint16_t hourNumber = 0;
       hourNumber < nvmHeader.hoursStored;
       hourNumber++)
  {
    if (!loggerDataReadHourBlock(hourNumber, hour))
      continue;


    const char* startTime = hour.startTime;
    for (uint8_t minute = 0; minute < 60; minute++)
    {
      const auto &m = hour.minute[minute];

      addMinutesToTimestamp(
        startTime,
        minute,
        timestamp,
        sizeof(timestamp));

      float average = 0.0f;

      if (m.count > 0)
        average = (float)m.total / m.count;

      csvPrintQuoted(out, timestamp);
      out.print(',');

      out.print(hourNumber);
      out.print(',');

      out.print(minute);
      out.print(',');

      out.print(m.count);
      out.print(',');

      out.print(m.total);
      out.print(',');

      out.print(m.shortest);
      out.print(',');

      out.print(m.longest);
      out.print(',');

      out.print(average, 1);
      out.print(',');

      csvPrintQuoted(out, getMinuteFlagText(m.flags));

      out.println();
    }
  }

  out.flush();
}


//------------------------------------------------------
// RENDER
//------------------------------------------------------

void renderExportCsv(Print &out) {
  // HTTP headers are emitted by the caller.
  // out.println(F("HTTP/1.1 200 OK"));
  // out.println(F("Content-Type: text/csv"));
  // out.println(F("Connection: close"));
  // out.println();

  out.print(F("# App Version,"));
  out.println(APP_VERSION);
  out.print(F("# Export Timestamp,"));
  out.println(getTimestamp());
  out.println();

  renderExportMainMetrics(out);
  renderExportLoggerData(out);
  renderExportSystemInfo(out);
  renderExportClockInfo(out);
  renderExportNvmInfo(out);
  renderExportLoggerData(out);
  renderExportRamLog(out);

}
