// export.cpp

#include "export.h"

// Project Headers
#include "global.h"
#include "ntp.h"
#include "nvm.h"
#include "pumpData.h"
#include "pumpFunc.h"
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


// -----------------------------------------------------------------------------
// MAIN DATA EXPORT
// -----------------------------------------------------------------------------
static void renderExportMainMetrics(WiFiClient &client) {
  client.println(F("[MAIN METRICS]"));

  client.print(F("pump_cycles,"));
  client.println(Pump.getPumpEventCount());

  client.print(F("pump_current,"));
  csvPrintQuoted(client, Pump.getLastEventSSCurText());
  client.println();

  client.print(F("last_cycle_energy_amp_seconds,"));
  client.println(String(Pump.getLastCycleEnergyAmpSeconds(), 2));

  client.print(F("last_cycle_min,"));
  client.println(String(Pump.getDeltaMin(), 2));

  client.print(F("avg_cycle_min,"));
  client.println(String(Pump.getAvgCycleMin(), 2));

  client.print(F("stdev_cycle_min,"));
  client.println(String(Pump.getStDevCycleMin(), 2));

  if (Pump.getPumpEventCount() >= 2) {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;

    const uint16_t  gallonsPerHour = (uint16_t)(5.0f * (60.0f / deltaMin));
    const uint32_t gallonsPerDayEst = (uint32_t)(5.0f * 60.0f * 24.0f / deltaMin);

    client.print(F("gallons_per_hour,"));
    client.println(gallonsPerHour);

    client.print(F("gallons_per_day_est,"));
    client.println(gallonsPerDayEst);
  }

  client.print(F("cycles_today,"));
  client.println(CYCLE_TODAY);

  client.print(F("gallons_today,"));
  client.println(GAL_TODAY);


  client.print(F("yesterday_was_zero,"));
  client.println(gYesterdayWasZero ? F("1") : F("0"));

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
// TREND INFO EXPORT
// -----------------------------------------------------------------------------
static void renderExportTrendInfo(WiFiClient &client) {
  addTitle(client, F("[TREND INFO]"));
  client.println(F("date,cycles_per_day,gallons_per_day"));

  const int recordCount = Pump.Daily365.dailyValidCount();
  for (int i = recordCount - 1; i >= 0; i--) {
    char date[16];
    PumpDailyRecord record;

    if (!Pump.Daily365.getDailyRecordAgo(i, record)) continue;
    getDayKeyForOffset(i + 1, date, sizeof(date));

    client.print(date);
    client.print(',');
    client.print(record.cyclesPerDay);
    client.print(',');
    client.print(record.gallonsPerDay);
  }
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
  renderExportTrendInfo(client);
  renderExportRamLog(client);

}
