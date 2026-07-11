This file is a merged representation of the entire codebase, combined into a single document by Repomix.

# File Summary

## Purpose
This file contains a packed representation of the entire repository's contents.
It is designed to be easily consumable by AI systems for analysis, code review,
or other automated processes.

## File Format
The content is organized as follows:
1. This summary section
2. Repository information
3. Directory structure
4. Repository files (if enabled)
5. Multiple file entries, each consisting of:
  a. A header with the file path (## File: path/to/file)
  b. The full contents of the file in a code block

## Usage Guidelines
- This file should be treated as read-only. Any changes should be made to the
  original repository files, not this packed version.
- When processing this file, use the file path to distinguish
  between different files in the repository.
- Be aware that this file may contain sensitive information. Handle it with
  the same level of security as you would the original repository.

## Notes
- Some files may have been excluded based on .gitignore rules and Repomix's configuration
- Binary files are not included in this packed representation. Please refer to the Repository Structure section for a complete list of file paths, including binary files
- Files matching patterns in .gitignore are excluded
- Files matching default ignore patterns are excluded
- Files are sorted by Git change count (files with more changes are at the bottom)

# Directory Structure
```
.gitignore
.repomixignore
datalogger/charts.cpp
datalogger/charts.h
datalogger/datalogger.ino
datalogger/datastore.cpp
datalogger/datastore.h
datalogger/diag.h
datalogger/eventData.cpp
datalogger/eventData.h
datalogger/export.cpp
datalogger/export.h
datalogger/global.h
datalogger/ledFunc.cpp
datalogger/ledFunc.h
datalogger/ntp.cpp
datalogger/ntp.h
datalogger/nvm.cpp
datalogger/nvm.h
datalogger/oled.cpp
datalogger/oled.h
datalogger/ota.cpp
datalogger/ota.h
datalogger/pump365.h
datalogger/pumpData.cpp
datalogger/pumpData.h
datalogger/pumpFunc.cpp
datalogger/pumpFunc.h
datalogger/ramlog.cpp
datalogger/ramLog.h
datalogger/server.cpp
datalogger/server.h
datalogger/test_mode.cpp
datalogger/test_mode.h
datalogger/testcode.cpp
datalogger/testcode.h
datalogger/utils.cpp
datalogger/utils.h
datalogger/wifiFunc.cpp
datalogger/wifiFunc.h
README-datalogger.md
repomix.config.json
```

# Files

## File: .gitignore
```
# -----------------------------
# Arduino / PlatformIO
# -----------------------------
*.ino.hex
*.ino.bin
*.elf
*.map

# Arduino build folders
build/
out/
.pio/

# -----------------------------
# IDE / OS files
# -----------------------------
.vscode/
.idea/
*.code-workspace

.DS_Store
Thumbs.db
ehthumbs.db
desktop.ini

# -----------------------------
# Logs & temp files
# -----------------------------
*.log
*.tmp
*.bak
*.swp

# -----------------------------
# Secrets (DO NOT COMMIT)
# -----------------------------
secrets.h
**/secrets.h

# -----------------------------
# OTHERS (DO NOT COMMIT)
# -----------------------------
GPT/
```

## File: .repomixignore
```
# -----------------------------
# Arduino / PlatformIO
# -----------------------------
*.ino.hex
*.ino.bin
*.elf
*.map

# Arduino build folders
build/
out/
.pio/

# -----------------------------
# IDE / OS files
# -----------------------------
.vscode/
.idea/
*.code-workspace

.DS_Store
Thumbs.db
ehthumbs.db
desktop.ini

# -----------------------------
# Logs & temp files
# -----------------------------
*.log
*.tmp
*.bak
*.swp

# -----------------------------
# Secrets (DO NOT COMMIT)
# -----------------------------
secrets.h
**/secrets.h
/pump_monitor/secrets.h

# -----------------------------
# OTHERS (DO NOT COMMIT)
# -----------------------------
repomix-pump.md
```

## File: datalogger/charts.cpp
```cpp
#include "charts.h"

#include "global.h"
#include "pumpData.h"
#include "eventData.h"
#include "ntp.h"
#include "utils.h"

namespace {

static void formatHourLabel(int hour24, char* out, size_t outLen) {
  const bool isPm = (hour24 >= 12);
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;
  snprintf(out, outLen, "%d%s", hour12, isPm ? "PM" : "AM");
}

}  // namespace

// -----------------------------------------------------------------------------
// Gallons / Day SVG chart
// -----------------------------------------------------------------------------
void renderGallonsPerDayChart(WiFiClient &client) {
  const int n = Pump.Daily365.dailyValidCount();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Gallons / Day (stored daily totals)</div>"));

  if (n <= 0) {
    client.println(F("<div class=\"small\">No daily totals saved yet.</div></div>"));
    return;
  }

  int maxG = -1;
  int minG = -1;

  for (int i = 0; i < n; i++) {
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;
    if (e.gallonsPerDay < 0  || e.gallonsPerDay > 10000) continue;

    if (i == 0) {
      maxG = e.gallonsPerDay;
      minG = e.gallonsPerDay;
    } else {
      if (e.gallonsPerDay > maxG) maxG = e.gallonsPerDay;
      if (e.gallonsPerDay < minG) minG = e.gallonsPerDay;
    }
  }

  // indicate if no valid values
  if (maxG < 0) {
    client.println(F("<div class=\"small\">No daily totals saved yet.</div></div>"));
    return;
  }

  uint32_t yMax = (uint32_t)(maxG * 1.1f + 50.0f);
  uint32_t yMin = (uint32_t)(minG * 0.9f);

  if (yMax < 100U) yMax = 100U;
  if (yMax <= yMin) yMax = yMin + 100U;

  const uint32_t range = yMax - yMin;

  uint32_t step;
  if (range < 900U)       step = 100U;
  else if (range < 2000U) step = 200U;
  else if (range < 2800U) step = 300U;
  else                    step = 500U;

  yMin = (yMin / step) * step;
  yMax = ((yMax + step - 1U) / step) * step;
  if (yMax <= yMin) yMax = yMin + step;

  const int W = 520, H = 180;
  const int padL = 38, padR = 12, padT = 16, padB = 28;
  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  const float denomY = (float)(yMax - yMin);

  for (uint32_t g = yMin; g <= yMax; g += step) {
    float t = ((float)g - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(padL);
    client.print(F("\" y1=\""));
    client.print(y);
    client.print(F("\" x2=\""));
    client.print(padL + plotW);
    client.print(F("\" y2=\""));
    client.print(y);
    client.println(F("\"/>"));

    client.print(F("<text text-anchor=\"end\" x=\"34\" y=\""));
    client.print(y + 4);
    client.print(F("\">"));
    client.print(g);
    client.println(F("</text>"));
  }

  client.print(F("<polyline class=\"line\" points=\""));

  for (int i = n - 1; i >= 0; i--) {
    const int idxChron = (n - 1) - i;
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;

    const int x = padL + (n == 1 ? plotW / 2 : (plotW * idxChron) / (n - 1));

    float t = ((float)e.gallonsPerDay - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(x); client.print(','); client.print(y); client.print(' ');
  }

  client.println(F("\"/>"));

  for (int i = n - 1; i >= 0; i--) {
    const int idxChron = (n - 1) - i;
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;

    const int x = padL + (n == 1 ? plotW / 2 : (plotW * idxChron) / (n - 1));

    float t = ((float)e.gallonsPerDay - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<circle class=\"pt\" cx=\""));
    client.print(x);
    client.print(F("\" cy=\""));
    client.print(y);
    client.println(F("\" r=\"3\"/>"));
  }

  char oldestDate[16];
  char newestDate[16];
  getDayKeyForOffset(n, oldestDate, sizeof(oldestDate));  // last day stored
  getDayKeyForOffset(1, newestDate, sizeof(newestDate));  // yesterday

  client.print(F("<text x=\""));
  client.print(padL);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(oldestDate);
  client.println(F("</text>"));

  client.print(F("<text x=\""));
  client.print(padL + plotW - 90);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(newestDate);
  client.println(F("</text>"));

  client.println(F("</svg></div>"));
}

// -----------------------------------------------------------------------------
// Current waveform SVG chart
// -----------------------------------------------------------------------------
void renderCurrentWaveChart(WiFiClient &client) {
  const int n = Pump.getLastCycleCurrentSamplesCount();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Last Cycle Current Waveform (A)</div>"));

  if (n <= 0) {
    client.println(F("<div class=\"small\">No last-cycle current samples yet.</div></div>"));
    return;
  }

  const float minA = Pump.getLastCycleCurrentMinAmps();
  const float maxA = Pump.getLastCycleCurrentMaxAmps();

  int yMinI = (int)(0.9f * minA);
  int yMaxI = (int)(1.1f * maxA + 0.5f);
  if (yMaxI <= yMinI) yMaxI = yMinI + 1;

  const float yMin = (float)yMinI;
  const float yMax = (float)yMaxI;

  const int W = 520;
  const int H = 180;
  const int padL = 44;
  const int padR = 12;
  const int padT = 16;
  const int padB = 28;

  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  const float denom = (yMax - yMin);
  for (int a = yMinI; a <= yMaxI; a++) {
    float t = ((float)a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(padL);
    client.print(F("\" y1=\""));
    client.print(y);
    client.print(F("\" x2=\""));
    client.print(padL + plotW);
    client.print(F("\" y2=\""));
    client.print(y);
    client.println(F("\"/>"));

    if ((a % 3) == 0) {
      client.print(F("<text text-anchor=\"end\" x=\""));
      client.print(padL - 6);
      client.print(F("\" y=\""));
      client.print(y + 4);
      client.print(F("\">"));
      client.print(a);
      client.println(F("</text>"));
    }
  }

  client.print(F("<polyline class=\"line\" points=\""));
  for (int i = 0; i < n; i++) {
    const float a = Pump.getLastCycleCurrentSampleAmps(i);
    const int x = padL + (n == 1 ? plotW / 2 : (plotW * i) / (n - 1));

    float t = (a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(x);
    client.print(',');
    client.print(y);
    client.print(' ');
  }
  client.println(F("\"/>"));

  for (int i = 0; i < n; i++) {
    const float a = Pump.getLastCycleCurrentSampleAmps(i);
    const int x = padL + (n == 1 ? plotW / 2 : (plotW * i) / (n - 1));

    float t = (a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<circle class=\"pt\" cx=\""));
    client.print(x);
    client.print(F("\" cy=\""));
    client.print(y);
    client.println(F("\" r=\"2\"/>"));
  }

  client.print(F("<text x=\""));
  client.print(padL);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.println(F("\">0</text>"));

  client.print(F("<text x=\""));
  client.print(padL + plotW - 24);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(n - 1);
  client.println(F("</text>"));

  client.println(F("</svg></div>"));
}

// -----------------------------------------------------------------------------
// Pump event 24h pulse chart
// -----------------------------------------------------------------------------
void renderPumpEvent24hChart(WiFiClient &client) {
  eventDataPurgeNow();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Pump Events - Last 24 Hours</div>"));

  const int eventCount = eventDataCount();
  if (eventCount <= 0) {
    client.println(F("<div class=\"small\">No pump events.</div></div>"));
    return;
  }

  const uint32_t nowMin2026 = eventDataGetMinutesSince2026();
  const uint32_t minutesSinceBootNow = minutesSinceBoot();
  uint32_t windowMinutes = minutesSinceBootNow;
  if (windowMinutes > 24U * 60U) windowMinutes = 24U * 60U;
  if (windowMinutes < 60U) windowMinutes = 60U;

  uint32_t windowHours = (windowMinutes + 59U) / 60U;
  if (windowHours < 1U) windowHours = 1U;

  uint32_t labelEveryHours;
  if (windowHours <= 4U)       labelEveryHours = 1U;
  else if (windowHours <= 12U) labelEveryHours = 2U;
  else                         labelEveryHours = 4U;

  const int W = 520;
  const int H = 90;

  const int padL = 40;
  const int padR = 12;
  const int padT = 10;
  const int padB = 24;

  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  const int x0 = padL;
  const int x1 = padL + plotW;

  const int yLow  = padT + plotH - 6;
  const int yHigh = padT + 8;

  // ---- adaptive pulse width ----
  int pulseWidthPx = 2;
  if (eventCount > 0) {
    const float spacing = (float)plotW / (float)eventCount;
    float w = spacing * 0.10f;

    if (w < 1.0f)  w = 1.0f;
    if (w > 10.0f) w = 10.0f;

    pulseWidthPx = (int)(w + 0.5f);
  }

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  // background
  client.print(F("<rect x=\""));
  client.print(x0);
  client.print(F("\" y=\""));
  client.print(padT);
  client.print(F("\" width=\""));
  client.print(plotW);
  client.print(F("\" height=\""));
  client.print(plotH);
  client.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\" stroke-width=\"1\"/>"));

  // baseline
  client.print(F("<line class=\"grid\" x1=\""));
  client.print(x0);
  client.print(F("\" y1=\""));
  client.print(yLow);
  client.print(F("\" x2=\""));
  client.print(x1);
  client.print(F("\" y2=\""));
  client.print(yLow);
  client.println(F("\"/>"));

  // ON/OFF labels
  client.print(F("<text text-anchor=\"end\" x=\""));
  client.print(padL - 6);
  client.print(F("\" y=\""));
  client.print(yHigh + 4);
  client.println(F("\">ON</text>"));

  client.print(F("<text text-anchor=\"end\" x=\""));
  client.print(padL - 6);
  client.print(F("\" y=\""));
  client.print(yLow + 4);
  client.println(F("\">OFF</text>"));

  // time grid
  const time_t nowEpoch = (time_t)getCurrentEpoch();
  struct tm nowTm;
  localtime_r(&nowEpoch, &nowTm);

  for (int hAgo = (int)windowHours; hAgo >= 0; hAgo -= (int)labelEveryHours) {
    const float frac = ((float)windowHours - (float)hAgo) / (float)windowHours;
    const int x = x0 + (int)(frac * (float)plotW + 0.5f);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(padT);
    client.print(F("\" x2=\""));
    client.print(x);
    client.print(F("\" y2=\""));
    client.print(padT + plotH);
    client.println(F("\"/>"));

    int labelHour = nowTm.tm_hour - hAgo;
    while (labelHour < 0) labelHour += 24;
    while (labelHour >= 24) labelHour -= 24;

    bool isPm = (labelHour >= 12);
    int hour12 = labelHour % 12;
    if (hour12 == 0) hour12 = 12;

    client.print(F("<text text-anchor=\"middle\" x=\""));
    client.print(x);
    client.print(F("\" y=\""));
    client.print(H - 6);
    client.print(F("\">"));
    client.print(hour12);
    client.print(isPm ? "PM" : "AM");
    client.println(F("</text>"));
  }

  // pulses
  for (int i = 0; i < eventCount; i++) {
    uint32_t evMin2026 = 0;
    if (!eventDataGetMinute(i, evMin2026)) continue;
    if (evMin2026 > nowMin2026) continue;

    const uint32_t ageMin = nowMin2026 - evMin2026;
    if (ageMin > windowMinutes) continue;

    const float frac = 1.0f - ((float)ageMin / (float)windowMinutes);
    const int x = x0 + (int)(frac * (float)plotW + 0.5f);
    const int x2 = (x + pulseWidthPx < x1) ? (x + pulseWidthPx) : x1;

    // vertical up
    client.print(F("<line x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(yLow);
    client.print(F("\" x2=\""));
    client.print(x);
    client.print(F("\" y2=\""));
    client.print(yHigh);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));

    // top
    client.print(F("<line x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(yHigh);
    client.print(F("\" x2=\""));
    client.print(x2);
    client.print(F("\" y2=\""));
    client.print(yHigh);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));

    // down
    client.print(F("<line x1=\""));
    client.print(x2);
    client.print(F("\" y1=\""));
    client.print(yHigh);
    client.print(F("\" x2=\""));
    client.print(x2);
    client.print(F("\" y2=\""));
    client.print(yLow);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));
  }

  client.println(F("</svg></div>"));
}
```

## File: datalogger/charts.h
```c
#pragma once

#include <WiFi.h>

void renderGallonsPerDayChart(WiFiClient &client);
void renderCurrentWaveChart(WiFiClient &client);
void renderPumpEvent24hChart(WiFiClient &client);
```

## File: datalogger/datalogger.ino
```
// Included Library Files
#include <Arduino.h>
#include <WiFi.h>

// My Libraries
#include "event.h"

// My Files
#include "global.h"
#include "datastore.h"
#include "diag.h"
#include "ledFunc.h"
#include "ntp.h"
#include "nvm.h"
#include "oled.h"
#include "ota.h"
#include "pumpData.h"
#include "pumpFunc.h"
#include "ramlog.h"
#include "server.h"
#include "testcode.h"
#include "test_mode.h"
#include "wifiFunc.h"

// Version Info
const char APP_VERSION[] PROGMEM = "V0.0A";

// System States
const char* CONN_STATUS = "OFF";
uint32_t LOOP_COUNT = 0;
uint32_t LOOP_TIME = 1000;
int ISR_CNT = 0;
unsigned int ONE_SEC_TIMER_MS = 0;
uint16_t ADAPTIVE_DELAY = 98;
unsigned int START_TIME = 0;

// Error Counts
int WIFI_ERR = 0;

// Hardware States
int ADC1_COUNT = 0;
float ADC1_VOLT = 0;
int BTN_VAL = 0;
int D1_VAL = 2;
int TS1_VAL = 0;

// NVM Boot Status
static bool gNvmRestoreDone = false;

// Events
Event testEvent;

// Diagnostics
Diag diagState;

// Binary Identifier
#if TEST_MODE
__attribute__((used)) static const char BUILD_MODE_MARKER[] PROGMEM =
    "PUMP_MONITOR|MODE=TEST";
#else
__attribute__((used)) static const char BUILD_MODE_MARKER[] PROGMEM =
    "PUMP_MONITOR|MODE=PROD";
#endif

const char* getBuildModeMarker() {
  return BUILD_MODE_MARKER;
}

void IRAM_ATTR isrFunction() {
  PUMP_EVENT = 1;
  ISR_CNT++;
}

void initNvmBootRestore() {
  if (gNvmRestoreDone) return;

  if (!timeValid()) {
    Serial.println("Time not valid yet; skipping NVM restore.");
    return;
  }

  char dayKey[16]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));

  // Update boot stats
  nvmUpdateBootStats(nowEpoch);

  gNvmRestoreDone = true;  // if we got here the restore was completed
}

void setup() {
  Serial.begin(115200);

  // Init ADC
  adcAttachPin(ADC1_PIN);
  //analogSetClockDiv(255); // 1338mS

  // Init LED
  pinMode(LED_PIN, OUTPUT);  // Set GPIO25 as digital output pin

  // Init ISR
  //pinMode(ISR_PIN, INPUT_PULLUP);
  //attachInterrupt(ISR_PIN, isrFunction, RISING);

  // Init OLED
  initDisplay();

  // Init NVM
  nvmInit();

  // Randomize
  randomSeed((uint32_t)esp_random());

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  Serial.println();
  Serial.println();
  Serial.print(F("### STARTING DATA LOGGER: "));
  Serial.print(FPSTR(APP_VERSION));   // FPSTR reads PROGMEM string
  Serial.println(F(" ###"));
  Serial.println("Setup Complete");
  //scanWifi();   // DISABLED – creates issues with Firebase TLS stability.  Only use for debugging board bring up
  Serial.println(getBuildModeMarker());
  connectWifi();

  // NTP Time Server
  newPopupScreen("NTP Setup", "Syncing NTP time");
  setupNTP();
  oledMain(MAIN_TIMEOUT_SEC);

  // Setup NVM Boot Restore  (must be after NTP)
  initNvmBootRestore();
  nvmDumpPumpState();

  // Init Pump
  initPump();

  // Store Startup Time
  START_TIME = (millis() / 1000); 

  // Test Functions
  //testDataStore(); 

  // Init OTA
  initOTA(TEST_MODE ? "pump-test" : "pump-prod");

  // Setup done, go to main screen
  oledMain(MAIN_TIMEOUT_SEC);
}

void loop() {
  static unsigned long loopCount = 0;

  webServer();   // SERVICE HTTP AS FAST AS POSSIBLE

  loop100ms();                              // Run the 100ms loop
  if (loopCount % 10 == 0) loop1Sec();      // Run the 1 sec loop
  if (loopCount % 100 == 0) loop10Sec();    // Run the 10 sec loop
  if (loopCount % 600 == 0) loop1Min();     // Run the 1 minute loop

  // Complete the loop
  loopCount++;
  LOOP_COUNT++;

  delay(ADAPTIVE_DELAY);  // avg code run time is ~2 ms
}

void loop100ms() {
  // Run Simulation if needed
  if (TEST_MODE) simulateLogger();

  // Read Inputs
  readDigitalButton();
  readADC();

  // Run Main Logic
  checkForPumpEvent();  // Want accurate timing so check every 100ms
  processPumpEvent();

  // Update Outputs
  displayText();
  updatePopupScreen();
  setLED();
  //webServer();  REMOVE AFRER TESTING

  // process Events
  processLoopCheck();
  processTestEvent();

  // Others
  handleOTA();
}

void loop1Sec() {
  static uint32_t count_1s;

  // Adaptive Loop Adjust
  static uint32_t prevLoopStart;
  uint32_t msNow = millis();
  ONE_SEC_TIMER_MS = msNow - prevLoopStart;
  prevLoopStart = msNow;

  // Dynamically Adjust Loop Time
  if (ONE_SEC_TIMER_MS < 1000 && ADAPTIVE_DELAY < 100) ADAPTIVE_DELAY++;
  else if (ONE_SEC_TIMER_MS > 1000 && ADAPTIVE_DELAY > 80) ADAPTIVE_DELAY--;


  // process 1 second tasks
  readDigital();
  readTouchSwitch();
  writeDAC();

  // Keep NTP Clock Valid
  if (wifiOK()) updateNTP();

  // update loop counter
  count_1s++;
}

void loop10Sec() {
  ensureServerStarted();

  // Early WiFi stabilization
  if (LOOP_COUNT < 40 * LOOPS_PER_SEC) checkWifi();


  // OTA
  void updateOTAState();

  // Make sure NVM was checked after boot
  if (!gNvmRestoreDone) initNvmBootRestore();  
}

void loop1Min() {
  static uint16_t minCount = 0;
  minCount = (minCount + 1) % 600;  // Prevent Overflow

  // Update System State
  if (WiFi.status() != WL_CONNECTED) diagState.setSystemState("*NO WIFI*");
  else if (!validClock()) diagState.setSystemState("*NO CLOCK*");
  else diagState.setSystemState("RUNNING");

  // Reconnect Wifi if needed
  checkWifi();
  updateWifiDiagState();
  diagState.updateDiagInfo();

  // Check NTP
  ensureTimeHealthy();

  // Run normal scheduled hourly tasks
  checkForOneHourTasks();
}

void checkForOneHourTasks() {
  static int8_t lastHour = -1;

  // Only run hourly logic if NTP/system time is valid
  if (!validClock()) return;

  const uint8_t hour = getHourInt();
  if (hour < 0 || hour > 23) return;

  // First valid hour after boot: initialize and do nothing
  if (lastHour < 0) {
    lastHour = hour;
    return;
  }

  // Detect a clean hour rollover (including 23 -> 0)
  const bool advanced =
    ((hour > 0 && lastHour == hour - 1) || (hour == 0 && lastHour == 23));

  if (hour != lastHour && advanced) {
    loop1Hour(hour);
    if (hour == 0) loop1DayMidnight();
    lastHour = hour;
  } 
  else if (hour != lastHour) {
    // Clock jumped (NTP correction / reboot / manual set) — resync without firing loops
    lastHour = hour;
  }
}

// One Hour Tasks - On the hour
void loop1Hour(int hour) {
  Serial.println();
  Serial.print("*** RUNNING 1 HOUR LOOP [");
  Serial.print(hour);
  Serial.println("]");

}

// Midnight
void loop1DayMidnight() {
  Serial.println();
  Serial.println("*** RUNNING 1 DAY LOOP - MIDNIGHT");

  // send daily email
  textStatus();

}

//------------------------------------------------------------------------

void writeDAC() {
  //dacWrite(DAC_PIN, LOOP_COUNT % 250);
}

void readADC() {
  uint32_t startTime = micros();

  if (TEST_MODE) ADC1_COUNT = getTestModeADC();
  else ADC1_COUNT = analogRead(ADC1_PIN);

  ADC1_VOLT = 3.3 * (ADC1_COUNT / 4095.0);
  TLog("ADC: ", ADC1_COUNT, startTime);
  TLog("ADC read time: ", startTime);
}

void readDigital() {
  uint32_t startTime = micros();
  D1_VAL = digitalRead(D1_PIN);
  TLog("Digital read time: ", startTime);
}

void readDigitalButton() {
  static uint8_t oldBtn = 1;
  BTN_VAL = digitalRead(BTN_PIN);     // Read New Button Status
  if (oldBtn == 1 && BTN_VAL == 0) processButton();
  oldBtn = BTN_VAL;                   // Save Status
}

void readTouchSwitch() {
  uint32_t startTime = micros();
  TS1_VAL = touchRead(TS1_PIN);
  TLog("TS read time: ", startTime);
}

void processButton() {
  Serial.println("Button Event");
  displayPopupScreen("BUTTON PRESSED", "Show Main Menu");
  delay(1000);
  if (wifiRadioOn()) {
    newPopupScreen("Turn OFF Wifi", "");
    disconnectWifi();
  } else {
    newPopupScreen("Turn ON Wifi", "");
    connectWifi();
  }
  delay(1000);
  oledMain(MAIN_TIMEOUT_SEC);
}

void processLoopCheck() {
  static uint32_t loopCount = 0;
  static uint32_t startTime = 0;
  if (loopCount % 999 == 0) {
    LOOP_TIME = int((millis() - startTime) / 100);
    Serial.println();
    if (LOOP_TIME < 950 || LOOP_TIME > 1050) {
      Serial.println("*** Total time for 1000 loops:" + String(LOOP_TIME));
    }
    startTime = millis();
  }
  loopCount++;
}

void processTestEvent() {
  if (testEvent.check()) {
    Serial.println();
    Serial.println("*** TEST EVENT ***");
    Serial.println("Total time for Test Event:" + String(testEvent.getDelta()) + "ms");
    Serial.println();
    testEvent.setSec(73);
  }
}

void testEmail() {
  //email.sendEmail("aneilshah@yahoo.com", "Sump Pump Status", "This will be data...\nSomeday");
  //email.sendEmail("7343555141@vtext.com", "Sump Pump", "Cycles: 5\nGallons Pumped: 600");
}

void textStatus() {
  float deltaMin = Pump.getAvgCycleMin();
  if (deltaMin < 0.1) deltaMin = 5.0;  // default to 5 mins if there is no data
  int cph = 60 / deltaMin;
  int cpd = 60 * 25 / deltaMin;
  int GPM = 48;  // Delete this after fixing
  int gallonsPerHour = 60 * GPM / deltaMin;   // WRONG - FIX THIS
  uint32_t gallonsPerDay = 60 * 24 * GPM / deltaMin;  // WRONG - FIX THIS

  String body = "Cycles: " + String(Pump.getPumpEventCount()) + "\n";

  float monMin = float(LOOP_COUNT / 60.0 / LOOPS_PER_SEC);
  float monDay = float(LOOP_COUNT / 3600.0 / 24 / LOOPS_PER_SEC);
  if (monDay < 1) {
    body += "Monitor Time: " + String(monMin) + "m\n";
  } else {
    body += "Monitor Time: " + String(monDay) + "d\n";
  }

  float pumpOnMin = float(Pump.getPumpOnTimeLoops() / 60.0 / LOOPS_PER_SEC);
  float pumpOnHour = float(Pump.getPumpOnTimeLoops() / 3600.0 / LOOPS_PER_SEC);
  if (pumpOnHour < 1) {
    body += "Pump Time: " + String(pumpOnMin) + "m\n";
  } else {
    body += "Pump Time: " + String(pumpOnHour) + "h\n";
  }
  body += "Current: " + Pump.getPumpCurText() + "\n";
  body += "Avg Cycle: " + String(Pump.getAvgCycleMin()) + "m\n";
  body += "StDev: " + String(Pump.getStDevCycleMin()) + "m\n";
  body += "CPD: " + String(cpd) + " / " + String(gallonsPerDay) + " Gal\n";
  body += "TS: " + String(getTimestamp()) + "\n";

  //email.sendEmail("aneilshah@yahoo.com", "Sump Pump Status", body);
  //email.sendEmail("7343555141@vtext.com", "Pump", "\n"+body);
}

void testDataStore() {
  delay(1000);
  DataStoreFloat X(15, 1, "abc");
  for (int i = 0; i <100; i++) {
    X.addData(i + (float(i)/100));
    Serial.println(X.dataText());
  }
}

void checkWifi() {
  if (!wifiOK()) {
    Serial.println("WiFi not OK, reconnecting...");
    connectWifi();
    WIFI_ERR++;
    return;
  }

  // 2) Optional: ensure WiFi stabilizes after reconnect
  if (!waitForWifiStable(1500, 5000)) {
    Serial.println("WiFi not stable yet");
    return;
  }
}

// Version History
// 0.0 Jul-09-2026 Initial Code
```

## File: datalogger/datastore.cpp
```cpp
#include "datastore.h"

// ################################################################################################
// CLASS DataStoreInt
// ################################################################################################

DataStoreInt::DataStoreInt(int size, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
  reset();
}

DataStoreInt::DataStoreInt() {
  _units = "";
  _maxCount = DATASTORE_MAX;
  reset();
}

float DataStoreInt::avg() {
  uint32_t sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

int DataStoreInt::getData(int i) const {
  if (i < 0 || i >= _dataCount) return 0;
  return _data[i];
}

float DataStoreInt::stdev() {
  float devSum = 0;
  float delta = 0;
  const float Avg = avg();

  for (int i = 0; i < _dataCount; i++) {
    delta = float(_data[i] - Avg);
    devSum += delta * delta;
  }

  float result = 0.0;
  if (_dataCount) result = float(sqrt(devSum / _dataCount));
  return result;
}

void DataStoreInt::reset() {
  _dataCount = 0;
  _totalCount = 0;
  for (int i = 0; i < _maxCount; i++) _data[i] = 0;
}

void DataStoreInt::init(int size, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
}

long DataStoreInt::getTotalCount() {return _totalCount;}

void DataStoreInt::addData(int x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
  _totalCount++;
}

String DataStoreInt::dataText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}

String DataStoreInt::htmlText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + "&ensp;";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}

// ################################################################################################
// CLASS DataStoreFloat
// ################################################################################################

DataStoreFloat::DataStoreFloat(int size, int prec, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _prec = prec;
  _units = units;
  reset();
}

DataStoreFloat::DataStoreFloat() {
  _units = "";
  _maxCount = DATASTORE_MAX;
  _prec = 1;
  reset();
}

float DataStoreFloat::avg() {
  float sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

float DataStoreFloat::stdev() {
  float devSum = 0;
  float delta = 0;
  const float Avg = avg();

  for (int i = 0; i < _dataCount; i++) {
    delta = float(_data[i] - Avg);
    devSum += delta * delta;
  }

  float result = 0.0;
  if (_dataCount) result = float(sqrt(devSum / _dataCount));
  return result;
}

void DataStoreFloat::reset() {
  _dataCount = 0;
  _totalCount = 0;
  for (int i = 0; i < _maxCount; i++) _data[i] = 0.0;
}

void DataStoreFloat::addData(float x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
  _totalCount++;
}

void DataStoreFloat::init(int size, int prec, String units) {
  if (size <= DATASTORE_MAX) _maxCount = size;
  else _maxCount = DATASTORE_MAX;
  _units = units;
  _prec = prec;
}

long  DataStoreFloat::getTotalCount() {return _totalCount;}

String DataStoreFloat::dataText() {
  float Avg = avg();
  float Stdev = stdev();
  String str = "";
  int Max = _dataCount;
  if (Max > DATASTORE_MAX) Max = DATASTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i],_prec) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());
  str += " | StDev:" + String(stdev());

  return str;
}


// ################################################################################################
// CLASS CurrentStoreInt
// ################################################################################################

CurrentStoreInt::CurrentStoreInt() {
  _units = "";
  _maxCount = CURRENTSTORE_MAX;
  reset();
}

float CurrentStoreInt::avg() {
  uint32_t sum = 0;

  for (int i = 0; i < _dataCount; i++) {
    sum += _data[i];
  }

  float result = 0.0;
  if (_dataCount) result = float(sum / _dataCount);
  return result;
}

float CurrentStoreInt::ssAvg(float tolPercent) { // Steady State average, tolPercent = 0.1 for 10%
  if (_dataCount <= 0) return 0.0f;

  // First-pass average (includes inrush/tail)
  const float a0 = avg();
  if (a0 <= 0.0f) return a0;  // avoid weirdness when idle/zero

  // Keep only samples within +/- tolPercent of the first average
  const float band = fabsf(a0) * tolPercent;

  uint32_t sum = 0;
  int n = 0;

  for (int i = 0; i < _dataCount; i++) {
    const float x = (float)_data[i];
    if (fabsf(x - a0) <= band) {
      sum += _data[i];
      n++;
    }
  }

  // If filter excluded everything (or almost everything), fall back to a0
  if (n <= 0) return a0;

  return (float)sum / (float)n;
}

void CurrentStoreInt::reset() {
  _dataCount = 0;
  lastDataText = dataText();
  lastHtmlText = htmlText();
  for (int i = 0; i < _maxCount; i++) _data[i] = 0;  // reset data
}

void CurrentStoreInt::addData(int x) {
  for (int i = _maxCount - 1; i > 0; i--) _data[i] = _data[i - 1]; 
  _data[0] = x;
  if (_dataCount < _maxCount) _dataCount++;
}

String CurrentStoreInt::dataText() {
  float Avg = avg();
  String str = "";
  int Max = _dataCount;
  if (Max > CURRENTSTORE_MAX) Max = CURRENTSTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + " ";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());

  return str;
}

String CurrentStoreInt::htmlText() {
  float Avg = avg();
  String str = "";
  int Max = _dataCount;
  if (Max > CURRENTSTORE_MAX) Max = CURRENTSTORE_MAX;

  str += "n:" + String(_dataCount) + " | ";
  for (int i = 0; i < Max; i++) {
    str += String(_data[i]) + "&ensp;";
  }
  if (_dataCount > 0) str += _units;
  str += " | AVG:" + String(avg());

  return str;
}

String CurrentStoreInt::getLastDataText() {return lastDataText;}
String CurrentStoreInt::getLastHtmlText() {return lastHtmlText;}
```

## File: datalogger/datastore.h
```c
#pragma once

#include "global.h"
#include <string.h>

#define DATASTORE_MAX 50
#define CURRENTSTORE_MAX 100

class DataStoreInt {
  public:
    DataStoreInt(int size, String units);
    DataStoreInt();
    float avg();
    float ssAvg(float tolPct = 0.10f);   // Steady State Avg
    float stdev();
    void reset();
    void addData(int x);
    long getTotalCount();
    String dataText();
    String htmlText();
    void init(int size, String units);
    int getData(int i) const;
    
  private:
    int _data[DATASTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    long _totalCount;
    String _units;
};


class DataStoreFloat {
  public:
    DataStoreFloat(int size, int prec, String units);
    DataStoreFloat();
    float avg();
    float stdev();
    void reset();
    void addData(float x);
    String dataText();
    long getTotalCount();
    void init(int size, int prec, String units);
    
  private:
    float _data[DATASTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    int _prec;
    long _totalCount;
    String _units;
};

class CurrentStoreInt {
  public:
    CurrentStoreInt();
    float avg();
    float ssAvg(float tolPercent = 0.2f);
    void reset();
    void addData(int x);
    String dataText();
    String htmlText();
    String getLastDataText();
    String getLastHtmlText();
    
  private:
    int _data[CURRENTSTORE_MAX+2];
    int _dataCount;
    int _maxCount;
    String _units;
    String lastDataText;
    String lastHtmlText;
};
```

## File: datalogger/diag.h
```c
#pragma once
#include <Arduino.h>
#include "ntp.h"

class Diag {
public:
  // Setters
  void setSystemState(const char* value) { setValue(systemState, value); }
  void setWifiState(const char* value) { setValue(wifiState, value); }
  void setWifiRSSI(const char* value) { setValue(wifiRSSI, value); }
  void setWifiIP(const char* value) { setValue(wifiIP, value); }
  void setWifiBSSID(const char* value) { setValue(wifiBSSID, value); }
  void setWifiDNS(const char* value) { setValue(wifiDNS, value); }
  void setWifiGW(const char* value) { setValue(wifiGW, value); }
  void setNTPState(const char* value) { setValue(ntpState, value); }
  void setPowerOnReason(const char* value) { setValue(powerOnReason, value); }
  void setHeapInfo(const char* value) { setValue(heapInfo, value); }
  void setNvmInfo(const char* value) { setValueLong(nvmInfo, value); }
  void setDiag1(const char* value) { setValue(diag1, value); }


  // Getters
  const char* getSystemState() const { return systemState; }
  const char* getWifiState() const { return wifiState; }
  const char* getWifiRSSI() const { return wifiRSSI; }
  const char* getWifiIP() const { return wifiIP; }
  const char* getWifiBSSID() const { return wifiBSSID; }
  const char* getWifiDNS() const { return wifiDNS; }
  const char* getWifiGW() const { return wifiGW; }
  const char* getNTPState() const { return ntpState; }

  // Updaters
  void updateDiagInfo() {
    // NTP
    if (!validClock()) {
      setNTPState("NTP_WAIT");
    } else {
      setNTPState("NTP_OK");
    }

    // Heap
    char buf[32];
    snprintf(buf, sizeof(buf), "%u / %u",
      ESP.getFreeHeap(),
      ESP.getMinFreeHeap());

    setHeapInfo(buf);
  } 

  Diag() {
    setDefault(systemState);
    setDefault(wifiState);
    setDefault(wifiRSSI);
    setDefault(wifiIP);
    setDefault(wifiBSSID);
    setDefault(wifiGW);
    setDefault(wifiDNS);
    setDefault(ntpState);
    setDefault(powerOnReason);
    setDefault(heapInfo);
    setDefault(nvmInfo);
    setDiag1("[unused]");
}

private:
  static const uint16_t DIAG_BUF_SIZE = 32;
  static const uint16_t DIAG_BUF_LONG = 160;

  char systemState[DIAG_BUF_SIZE];
  char wifiState[DIAG_BUF_SIZE];
  char wifiRSSI[DIAG_BUF_SIZE];
  char wifiIP[DIAG_BUF_SIZE];
  char wifiBSSID[DIAG_BUF_SIZE];
  char wifiGW[DIAG_BUF_SIZE];
  char wifiDNS[DIAG_BUF_SIZE];
  char ntpState[DIAG_BUF_SIZE];
  char powerOnReason[DIAG_BUF_SIZE];
  char heapInfo[DIAG_BUF_SIZE];
  char diag1[DIAG_BUF_SIZE];
  char nvmInfo[DIAG_BUF_LONG];

  char fbState[DIAG_BUF_SIZE];
  char fbError[64];
  char fbTimeout[DIAG_BUF_SIZE];

  void setValue(char* dest, const char* src) {
    if (src) {
        strncpy(dest, src, DIAG_BUF_SIZE - 1);
        dest[DIAG_BUF_SIZE - 1] = '\0';
    } else {
        dest[0] = ' ';
        dest[1] = '\0';
    }
  }

  void setValueLong(char* dest, const char* src) {
      if (src) {
          strncpy(dest, src, DIAG_BUF_LONG - 1);
          dest[DIAG_BUF_LONG - 1] = '\0';
      } else {
          dest[0] = ' ';
          dest[1] = '\0';
      }
  }

  void setDefault(char* dest) {
      dest[0] = '\0';
  }
};

extern Diag diagState;
```

## File: datalogger/eventData.cpp
```cpp
#include "eventData.h"

#include <string.h>
#include <time.h>

#include "ntp.h"

namespace {

class EventData {
public:
  static const int MAX_EVENTS = 250;
  static const uint32_t WINDOW_MINUTES = 24U * 60U;

  EventData() {
    clear();
  }

  void clear() {
    memset(eventMinute_, 0, sizeof(eventMinute_));
    head_ = 0;
    count_ = 0;
  }

  bool addNow() {
    if (!validClockNow()) return false;

    const uint32_t minute2026 = getMinutesSince2026Now();
    if (minute2026 == 0) return false;

    addMinute(minute2026);
    return true;
  }

  void addMinute(uint32_t minute2026) {
    // De-dupe: ignore repeated events in same minute
    if (count_ > 0) {
      uint32_t newestMinute = 0;
      if (get(count_ - 1, newestMinute) && newestMinute == minute2026) {
        purgeOld(minute2026);
        return;
      }
    }

    eventMinute_[head_] = minute2026;
    head_ = (head_ + 1) % MAX_EVENTS;

    if (count_ < MAX_EVENTS) {
      count_++;
    }

    purgeOld(minute2026);
  }

  void purgeOld(uint32_t nowMinute2026) {
    while (count_ > 0) {
      const int idx = oldestIndex();
      const uint32_t eventMinute = eventMinute_[idx];

      // Guard bad clock jumps
      if (eventMinute > nowMinute2026) {
        break;
      }

      const uint32_t ageMinutes = nowMinute2026 - eventMinute;
      if (ageMinutes <= WINDOW_MINUTES) {
        break;
      }

      count_--;
    }
  }

  int count() const {
    return count_;
  }

  bool empty() const {
    return count_ == 0;
  }

  bool get(int logicalIndex, uint32_t& outMinute2026) const {
    if (logicalIndex < 0 || logicalIndex >= count_) return false;

    outMinute2026 = eventMinute_[physicalIndex(logicalIndex)];
    return true;
  }

  static bool validClockNow() {
    return validClock() && getCurrentEpoch() > 0;
  }

  static uint32_t getMinutesSince2026Now() {
    const uint32_t nowEpoch = getCurrentEpoch();
    if (nowEpoch == 0) return 0;

    struct tm baseTm;
    memset(&baseTm, 0, sizeof(baseTm));
    baseTm.tm_year = 2026 - 1900;
    baseTm.tm_mon = 0;
    baseTm.tm_mday = 1;

    const time_t baseEpoch = mktime(&baseTm);
    if (baseEpoch <= 0) return 0;
    if (nowEpoch <= (uint32_t)baseEpoch) return 0;

    return (nowEpoch - (uint32_t)baseEpoch) / 60U;
  }

private:
  uint32_t eventMinute_[MAX_EVENTS];
  int head_;
  int count_;

  int oldestIndex() const {
    return (head_ - count_ + MAX_EVENTS) % MAX_EVENTS;
  }

  int physicalIndex(int logicalIndex) const {
    return (oldestIndex() + logicalIndex) % MAX_EVENTS;
  }
};

static EventData gEventData;

} // namespace

// ---- Public API ----

void eventDataInit() {
  gEventData.clear();
}

void eventDataClear() {
  gEventData.clear();
}

bool eventDataAddNow() {
  return gEventData.addNow();
}

void eventDataAddMinute(uint32_t minute2026) {
  gEventData.addMinute(minute2026);
}

int eventDataCount() {
  return gEventData.count();
}

bool eventDataEmpty() {
  return gEventData.empty();
}

bool eventDataGetMinute(int index, uint32_t& outMinute2026) {
  return gEventData.get(index, outMinute2026);
}

bool eventDataValidClock() {
  return EventData::validClockNow();
}

uint32_t eventDataGetMinutesSince2026() {
  return EventData::getMinutesSince2026Now();
}

void eventDataPurgeNow() {
  uint32_t now = EventData::getMinutesSince2026Now();
  if (now == 0) return;

  gEventData.purgeOld(now);
}
```

## File: datalogger/eventData.h
```c
#pragma once

#include <stdint.h>

// Lifecycle
void eventDataInit();
void eventDataClear();

// Add events
bool eventDataAddNow();
void eventDataAddMinute(uint32_t minute2026);
void eventDataPurgeNow();


// Info
int eventDataCount();
bool eventDataEmpty();

// Access (0 = oldest, count-1 = newest)
bool eventDataGetMinute(int index, uint32_t& outMinute2026);

// Helpers
bool eventDataValidClock();
uint32_t eventDataGetMinutesSince2026();
```

## File: datalogger/export.cpp
```cpp
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
// BLOCK EXPORT
// -----------------------------------------------------------------------------
static void renderExportBlockData(WiFiClient &client) {
  addTitle(client, F("[BLOCKS]"));

  client.print(F("total_block_writes,"));
  client.printf("%u / 6\n", getTotalBlockWriteCount());

  client.print(F("daily_block_day,"));
  client.println(blockDayKey);

  client.print(F("cycles"));
  client.print(F(","));
  client.println(CYCLE);

  client.print(F("gallons"));
  client.print(F(","));
  client.println(GAL);

  client.print(F("cycles_today,"));
  client.println(CYCLE_TODAY);

  client.print(F("gallons_today,"));
  client.println(GAL_TODAY);
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
  renderExportBlockData(client);
  renderExportSystemInfo(client);
  renderExportClockInfo(client);
  renderExportNvmInfo(client);
  renderExportTrendInfo(client);
  renderExportRamLog(client);

}
```

## File: datalogger/export.h
```c
#pragma once

#include <WiFi.h>

// Main export entry point
void renderExportCsv(WiFiClient &client);
void renderExportJson(WiFiClient &client);
```

## File: datalogger/global.h
```c
// global.h
#pragma once

#include <Arduino.h>

class Diag;   // forward declaration

// external vaiables
extern int ADC1_COUNT;
//extern float ADC1_VOLT; 
extern const char* CONN_STATUS;
extern uint32_t LOOP_COUNT;
extern uint32_t LOOP_TIME;
extern int WIFI_ERR;
extern const char APP_VERSION[];     // (don’t put PROGMEM on the extern)

// Configurations

//---------------------------------
#define TEST_MODE 1
//---------------------------------

// Logging /Debug
#define VERBOSE 0
#define LOG_TIME 0  // serial log for execution times
#define ALLOW_WEBPAGE_POLLING 0

// Constants
#define OFF 0
#define ON 1
#define FALSE 0
#define TRUE 1

#define SENSOR_AMP_PER_VOLT 10
#define LOOPS_PER_SEC 10

// OLED MAIN MENU MODE
#define MAIN_TIMEOUT_SEC (900 * LOOPS_PER_SEC)  // 15 min

// PIN MAPPINGS
#define ADC1_PIN 35
#define BTN_PIN 0
#define D1_PIN 0
//#define ISR_PIN 35
#define LED_PIN 25 // OLD Board 25, new board 35
#define TS1_PIN 14  // Touch Switch
#define ISR_PIN 35

// Computed Values (DO NOT EDIT)
#define PROD_MODE (TEST_MODE == 0)

// Needs to be last
#include "utils.h"
```

## File: datalogger/ledFunc.cpp
```cpp
#include "global.h"
#include "ledFunc.h"

uint8_t LED_STATE = LED_OFF;

void toggleLED() {
  LED_STATE = !LED_STATE;
  setLED();
}

void LEDOn() {
  LED_STATE = LED_ON;
  setLED();
}

void LEDOff() {
  LED_STATE = LED_OFF;
  setLED();
}

void setLED() {
  digitalWrite(LED_PIN, LED_STATE);  // LED
}
```

## File: datalogger/ledFunc.h
```c
#pragma once

#define LED_ON 1
#define LED_OFF 0

extern uint8_t LED_STATE;

void toggleLED();
void LEDOn();
void LEDOff();
void setLED();
```

## File: datalogger/ntp.cpp
```cpp
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

// UNIQUE TO PUMP - MERGE LATER

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
```

## File: datalogger/ntp.h
```c
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

// Unique to Pump (MERGE LATER)
void getCurrentDayKey(char* buf, size_t len);
uint32_t getCurrentEpoch();
```

## File: datalogger/nvm.cpp
```cpp
// nvm.cpp
#include <Preferences.h>
#include <string.h>
#include "global.h"
#include "ntp.h"
#include "nvm.h"
#include "pumpFunc.h"

static Preferences prefs;

// Namespaces
static const char* NS_PUMP = "pump";
static const char* NS_BOOT = "boot";

// Keys (pump namespace)
static const char* K_HAS_STATE  = "has_state";
static const char* K_DAY_KEY    = "day_key";
static const char* K_SAVE_EPOCH = "save_epoch";
static const char* K_GAL        = "gal";
static const char* K_CYC        = "cyc";
static const char* K_WRITE_COUNT = "write_count";  // Export this one

// Keys (boot namespace)
static const char* K_BOOT_COUNT     = "boot_count";
static const char* K_LAST_BOOT_EPOCH = "last_boot_epoch";
static const char* K_PREV_BOOT_EPOCH = "prev_boot_epoch";


void nvmInit() {
  // Nothing required. Preferences/NVS is ready by default on ESP32.
  // This exists just to give you a clean "init hook".
}

static bool safeStrEq(const String& a, const char* b) {
  return b && a.equals(b);
}

//-------------------------------------------------
// 4 Hour Blocks
//-------------------------------------------------

bool nvmSaveState(
  const char* dayKey,
  uint32_t saveEpoch,
  uint32_t gal,
  uint32_t cyc
) {
  if (!dayKey) return false;

  if (!prefs.begin(NS_PUMP, false)) return false;
  
  // Get updated write counts
  uint32_t oldWriteCount = prefs.getULong(K_WRITE_COUNT, 0);
  uint32_t newWriteCount = oldWriteCount + 1;  // incremenet explicitly
  uint8_t hour = getHourInt();

  bool ok = true;
  ok &= prefs.putBool(K_HAS_STATE, true);
  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, (unsigned long)saveEpoch) > 0);
  ok &= (prefs.putULong(K_WRITE_COUNT, newWriteCount) > 0);
  ok &= (prefs.putULong(K_GAL, gal) > 0);
  ok &= (prefs.putULong(K_CYC, cyc) > 0);

  prefs.end();

  if (ok) snprintf(blockDayKey, BLOCK_DAY_KEY_SIZE, "%s", dayKey);
  return ok;
}

void nvmClearState() {  
  if (!prefs.begin(NS_PUMP, false)) return;

  // Dont remove K_WRITE_COUNT
  prefs.remove(K_HAS_STATE);
  prefs.remove(K_DAY_KEY);
  prefs.remove(K_SAVE_EPOCH);
  prefs.remove(K_GAL);
  prefs.remove(K_CYC);
  prefs.end();
}

bool nvmSetZeroBlocks() {
  bool ok = true;

  if (!prefs.begin(NS_PUMP, false)) {
    return false;
  }

  // Default / clean values
  char dayKey[BLOCK_DAY_KEY_SIZE]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));
  const uint32_t zeroGal = 0;
  const uint32_t zeroCyc = 0;

  ok &= prefs.putBool(K_HAS_STATE, true);

  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, nowEpoch) > 0);

  // Dont change K_WRITE_COUNT

  ok &= (prefs.putULong(K_GAL, zeroGal) > 0);
  ok &= (prefs.putULong(K_CYC, zeroCyc) > 0);

  prefs.end();
  return ok;
}

//-------------------------------------------------
// NVM Getters
//-------------------------------------------------

uint32_t getTotalBlockWriteCount() {
  if (!prefs.begin(NS_PUMP, true)) {return 0;}
  const uint32_t v = (uint32_t)prefs.getULong(K_WRITE_COUNT, 0);
  prefs.end();
  return v;
}

//-------------------------------------------------
// BOOT INFO
//-------------------------------------------------

void nvmUpdateBootStats(uint32_t nowEpoch) {
  if (!prefs.begin(NS_BOOT, false)) return;

  const uint32_t bootCount = (uint32_t)prefs.getUInt(K_BOOT_COUNT, 0);
  const uint32_t lastBoot  = (uint32_t)prefs.getUInt(K_LAST_BOOT_EPOCH, 0);

  prefs.putUInt(K_BOOT_COUNT, bootCount + 1);
  prefs.putUInt(K_PREV_BOOT_EPOCH, lastBoot);
  prefs.putUInt(K_LAST_BOOT_EPOCH, nowEpoch);

  prefs.end();
}

uint32_t nvmGetBootCount() {
  if (!prefs.begin(NS_BOOT, true)) return 0;
  const uint32_t v = (uint32_t)prefs.getUInt(K_BOOT_COUNT, 0);
  prefs.end();
  return v;
}

uint32_t nvmGetLastBootEpoch() {
  if (!prefs.begin(NS_BOOT, true)) return 0;
  const uint32_t v = (uint32_t)prefs.getUInt(K_LAST_BOOT_EPOCH, 0);
  prefs.end();
  return v;
}

uint32_t nvmGetPrevBootEpoch() {
  if (!prefs.begin(NS_BOOT, true)) return 0;
  const uint32_t v = (uint32_t)prefs.getUInt(K_PREV_BOOT_EPOCH, 0);
  prefs.end();
  return v;
}

void nvmDumpPumpState()
{
  Serial.println("----- NVM PUMP DUMP BEGIN -----");

  if (!prefs.begin(NS_PUMP, true)) {
    Serial.println("NVM dump failed: prefs.begin(NS_PUMP) failed");
    return;
  }

  bool hasState = prefs.getBool(K_HAS_STATE, false);
  String dayKey = prefs.getString(K_DAY_KEY, "");
  uint32_t saveEpoch = (uint32_t)prefs.getULong(K_SAVE_EPOCH, 0);

  Serial.printf("has_state: %s\n", hasState ? "true" : "false");
  Serial.printf("day_key:   %s\n", dayKey.c_str());
  Serial.printf("epoch:     %lu\n", (unsigned long)saveEpoch);

  uint32_t gal = prefs.getULong(K_GAL, 0);
  uint32_t cyc = prefs.getULong(K_CYC, 0);
  Serial.printf("  gal=%lu cyc=%lu\n", gal, cyc);
  prefs.end();

  Serial.println("----- NVM PUMP DUMP END -----");
}

void nvmDumpBootState()
{
  Serial.println("----- NVM BOOT DUMP BEGIN -----");

  if (!prefs.begin(NS_BOOT, true)) {
    Serial.println("NVM dump failed: prefs.begin(NS_BOOT) failed");
    return;
  }

  uint32_t bootCount = prefs.getUInt(K_BOOT_COUNT, 0);
  uint32_t lastBoot  = prefs.getUInt(K_LAST_BOOT_EPOCH, 0);
  uint32_t prevBoot  = prefs.getUInt(K_PREV_BOOT_EPOCH, 0);

  Serial.printf("boot_count:      %lu\n", (unsigned long)bootCount);
  Serial.printf("last_boot_epoch: %lu\n", (unsigned long)lastBoot);
  Serial.printf("prev_boot_epoch: %lu\n", (unsigned long)prevBoot);

  prefs.end();
  Serial.println("----- NVM BOOT DUMP END -----");
}
```

## File: datalogger/nvm.h
```c
// nvm.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#define INVALID_BLOCK 0  // WANT THIS TO BE 255

// ---- Lifecycle ----
void nvmInit();   // optional; safe to call once in setup()

// ----  block state ----
// Saves: has_state, day_key, save_epoch,  GAL, CYCLE
bool nvmSaveState(
  const char* dayKey,           // "YYYY_MM_DD"
  uint32_t saveEpoch,           // epoch seconds (0 ok, but restore-window won't work)
  uint32_t gal, 
  uint32_t cyc
);


//  Block Methods
void nvmClearState();
void nvmDumpPumpState();
bool nvmSetZeroBlocks();

// Getters
uint32_t getTotalBlockWriteCount();


// ---- Boot stats (update once per reboot) ----
// Increments boot_count and updates last_boot_epoch / prev_boot_epoch.
void nvmUpdateBootStats(uint32_t nowEpoch);

// Read boot stats (optional convenience)
uint32_t nvmGetBootCount();
uint32_t nvmGetLastBootEpoch();
uint32_t nvmGetPrevBootEpoch();
```

## File: datalogger/oled.cpp
```cpp
// Library Files
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "global.h"
#include "oled.h"
#include "ntp.h"
#include "pumpData.h"
#include "pumpFunc.h"
#include "wifiFunc.h"

// Just in case
#include <stdio.h>
#include <string.h>

// Global Variables
#include "global.h"

// Display
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

// OLED State
unsigned int OLED_MODE;
static char PopupText[32] = "";
static char PopupDetails[64] = "";
uint32_t PopupTimer = 0;
uint32_t MainTimer = 0;
uint32_t MainTimeout = OLED_MODE_NO_TIMEOUT;

#define MIN_MODE_CYCLE_TIME (5 * LOOPS_PER_SEC)

//-------------------------------------------
// HELPERS
//-------------------------------------------

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void initDisplay() {
  display.init();
  OLED_MODE = OLED_OFF;
  VextON();
}

void displayText() {
  static unsigned int cnt = 0;
  static unsigned int horOffset = 0;
  static unsigned int vertOffset = 0;
  static unsigned int vertOffsetMain = 0;

  // Local scratch buffers
  char line0[48];
  char line1[48];
  char line2[48];
  char line3[48];
  char line4[48];
  char line5[48];

  horOffset = (cnt / (60 * LOOPS_PER_SEC)) % 10;  // Every 60 seconds, span = 10 pixels  

  if (OLED_MODE == OLED_MAIN) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Line 0: HEADER
    if (TEST_MODE) snprintf(line0, sizeof(line0), "SHAH LOG %s T", APP_VERSION);
    else           snprintf(line0, sizeof(line0), "SHAH LOG %s", APP_VERSION);
    display.drawString(horOffset, vertOffsetMain, line0);

    // Line 1: Last Cycle
    float lastCycle = Pump.getDeltaMin();
    if (lastCycle <= 0.1f) {
      snprintf(line1, sizeof(line1), "Last Cycle: --");
    } else {
      snprintf(line1, sizeof(line1), "Last Cycle: %.1fm", lastCycle);
    }
    display.drawString(horOffset, 10 + vertOffsetMain, line1);

    // Line 2: CPH / CPD
    float avgCycleMin = Pump.getAvgCycleMin();
    float cph = 0.0f;
    int cpd = 0;

    if (avgCycleMin > 0.1f) {
      cph = 60.0f / avgCycleMin;
      cpd = (int)(cph * 24.0f);
    }
    char curText[12];
    Pump.getPumpCurText(curText, sizeof(curText));

    snprintf(line2, sizeof(line2), "CPH: %.1f  CPD: %d", cph, cpd);
    display.drawString(horOffset, 20 + vertOffsetMain, line2);


    // Line 3: Gallons - Current
    int gallonsPerDay = 0;
    if (avgCycleMin > 0.1f) {
      gallonsPerDay = (int)(5 * 60 * 24 / avgCycleMin);
    }
    if (avgCycleMin <= 0.1f) {
      snprintf(line3, sizeof(line3), "GPD: --  I: %s", curText);
    } else {
      snprintf(line3, sizeof(line3), "GPD: %d  I: %s", gallonsPerDay, curText);
    }
    display.drawString(horOffset, 30 + vertOffsetMain, line3);


    /////////////////////////////////////////
    // Line 4: Revolving Line
    /////////////////////////////////////////
    const float TimePerStage = 2.0; // Seconds
    const int Stages = 3;
    const int TotalTime = LOOPS_PER_SEC * TimePerStage * Stages;
    const int TimePerStageLoops = int(TimePerStage * LOOPS_PER_SEC);

    if (LOOP_COUNT % TotalTime < TimePerStageLoops) {
      // Pct ON / OFF
      snprintf(line4, sizeof(line4), "<RESERVED 1>");
    }
    else if (LOOP_COUNT % TotalTime < 2 * TimePerStageLoops) {
      // WiFi status
      if (wifiRadioOn())
        snprintf(line4, sizeof(line4), "WIFI: %s", CONN_STATUS);
      else
        snprintf(line4, sizeof(line4), "WIFI: RADIO OFF");
    }
    else {
      snprintf(line4, sizeof(line4), "<RESERVED 2>");
    }

    display.drawString(horOffset, 40 + vertOffsetMain, line4);

    // Line 5: On Time / Clock
    float hours = cnt / 36000.0f;
    if (hours < 24.0f) {
      snprintf(line5, sizeof(line5), "UP:%.1fh | CLK:%s", hours, getClock());
    } else {
      float days = hours / 24.0f;
      snprintf(line5, sizeof(line5), "UP:%.1fd | CLK:%s", days, getClock());
    }
    display.drawString(horOffset, 50 + vertOffsetMain, line5);

    // write the buffer to the display
    display.display();

    // Update timers and change state as needed
    if (MainTimer > 0) MainTimer--;
    if (MainTimeout != OLED_MODE_NO_TIMEOUT && MainTimer == 0) {
      OLED_MODE = OLED_MINIMIZED;  
    }
  }

else if (OLED_MODE == OLED_MINIMIZED) {
  float deltaMin = Pump.getAvgCycleMin();
  if (deltaMin < 0.1f) deltaMin = 5.0f;

  uint32_t gallonsPerDay = (uint32_t)(5 * 60 * 24 / deltaMin);

  char cycMin[10];
  snprintf(cycMin, sizeof(cycMin), "%.1f", deltaMin);

  const unsigned int CYCLE_COUNT = 8;
  unsigned int mode = (cnt / MIN_MODE_CYCLE_TIME) % CYCLE_COUNT;

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  if (mode == 0) {
    if (TEST_MODE) snprintf(line0, sizeof(line0), "*TEST MODE* %s", APP_VERSION);
    else           snprintf(line0, sizeof(line0), "DATA APP %s", APP_VERSION);
    display.drawString(horOffset, vertOffset, line0);
  }
  else if (mode == 1) {
    snprintf(line2, sizeof(line2), "ON: %u%c [%u%c]",
            (unsigned)(cnt / LOOPS_PER_SEC), 's',
            (unsigned)(cnt / 864000), 'd');
    display.drawString(horOffset, vertOffset, line2);
  }
  else if (mode == 2) {
    snprintf(line2, sizeof(line2), "CYC: %sm %ld GPD", cycMin, gallonsPerDay);
    display.drawString(horOffset, vertOffset, line2);
  }
  else if (mode == 3) {
    snprintf(line2, sizeof(line2), "CLOCK: %s", getClock());
    display.drawString(horOffset, vertOffset, line2);
  }
  else if (mode == 4) {
    snprintf(line2, sizeof(line2), "DATE: %s", getDate());
    display.drawString(horOffset, vertOffset, line2);
  }
  else if (mode == 5) {
    snprintf(line2, sizeof(line2), "YEAR: %s", getYearStr());
    display.drawString(horOffset, vertOffset, line2);
  }
  else if (mode == 6) {
    snprintf(line2, sizeof(line2), "Wifi: %s", CONN_STATUS);
    display.drawString(horOffset, vertOffset, line2);
  }
  else {
    display.drawString(horOffset, vertOffset, "Press TOP Button");
  }

  if (cnt % (120 * LOOPS_PER_SEC) == 0) vertOffset = random(0, 50);

  display.display();
}

  cnt++;  // shared counter for all modes
}

void displayPopupScreen(const char* text, const char* details) {
  strncpy(PopupText, text, sizeof(PopupText) - 1);
  PopupText[sizeof(PopupText) - 1] = '\0';

  strncpy(PopupDetails, details, sizeof(PopupDetails) - 1);
  PopupDetails[sizeof(PopupDetails) - 1] = '\0';

  PopupTimer = 0;
  OLED_MODE = OLED_POPUP;
  updatePopupScreen();
}

void updatePopupScreen() {
  if (OLED_MODE == OLED_POPUP) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, PopupText);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 16, PopupDetails);

    // write the buffer to the display
    display.display();

    PopupTimer++;
  }
}

void newPopupScreen(const char* text, const char* details) {
  displayPopupScreen(text,details);
  updatePopupScreen();
}

void oledMain(uint32_t duration) {
  OLED_MODE = OLED_MAIN;
  MainTimer = duration;
  MainTimeout = duration;
}

void oledMinimized() {
  OLED_MODE = OLED_MINIMIZED;
}

void oledOff() {
  OLED_MODE = OLED_OFF;
}
```

## File: datalogger/oled.h
```c
#pragma once

void initDisplay();
void displayText();
void displayPopupScreen(const char* text, const char* details);
void newPopupScreen(const char* text, const char* details);
void updatePopupScreen();
void oledMain(uint32_t duration);
void oledMinimized();
void oledOff();

#define OLED_OFF 0
#define OLED_MAIN 1
#define OLED_POPUP 2
#define OLED_MINIMIZED 3

#define OLED_MODE_NO_TIMEOUT 0
```

## File: datalogger/ota.cpp
```cpp
// ota.cpp
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "ota.h"
#include "global.h"
#include "wifiFunc.h"

static bool _otaInProgress = false;
static unsigned int otaState = OTA_OFF;

unsigned int getOtaState() {return otaState;}
void reinitOta() {otaState = OTA_INIT;}

void initOTA(const char* hostname)
{
  if (!MDNS.begin(hostname)) {
    Serial.println("mDNS failed");
  } else {
    Serial.println("mDNS started");
  }
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(3232);

  // Optional but strongly recommended
  //ArduinoOTA.setPassword("pumpOTA123");   // change this!

  ArduinoOTA.onStart([]() {
    _otaInProgress = true;
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
    _otaInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static int16_t lastPct = -5;

    int pct = (progress * 100) / total;

    if (pct - lastPct >= 5 || pct == 100) {
      Serial.printf("OTA Progress: %u%%\n", pct);
      lastPct = pct;
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      else Serial.println("Unknown");
      _otaInProgress = false;
    });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Host: %s\n", hostname);
  Serial.println("[OTA] Ready");
}

void updateOTAState() {
  bool wifiStable = wifiLinkReady();  

  switch (otaState) {
    case OTA_OFF:
      if (wifiStable) {
        Serial.println("[OTA] OFF -> ON");
        initOTA(TEST_MODE ? "pump-test" : "pump-prod");
        otaState = OTA_ON;
      }
      break;

    case OTA_ON:
      if (!wifiStable) {
        Serial.println("[OTA] ON -> INIT");
        otaState = OTA_INIT;
      }
      break;

    case OTA_INIT:
      if (wifiStable) {
        Serial.println("[OTA] INIT -> ON");
        initOTA(TEST_MODE ? "pump-test" : "pump-prod");
        otaState = OTA_ON;
      }
      break;
  }
}

void handleOTA()
{
  ArduinoOTA.handle();
}

bool otaInProgress()
{
  return _otaInProgress;
}
```

## File: datalogger/ota.h
```c
// OTA.h
#pragma once

#define OTA_OFF 0
#define OTA_INIT 1
#define OTA_ON 2
#define OTA_FLASHING 3

// OTA Handler
void initOTA(const char* hostname);
void handleOTA();
bool otaInProgress();

// OTA State
unsigned int getOtaState();
void reinitOta();
void updateOTAState();
```

## File: datalogger/pump365.h
```c
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
```

## File: datalogger/pumpData.cpp
```cpp
#include "global.h"
#include "eventData.h"
#include "pumpData.h"
#include <string.h>

// ################################################################################################
// CLASS PumpData
// ################################################################################################

PumpData::PumpData() {
  DELTA_SEC = 6000;  // default 10mins
  DELTA_MIN = 10;    // default 10 mins
  AVG_PUMP_CUR_COUNT = 0;
  PUMP_EVENT_COUNT = 0;
  LAST_PUMP_ON_TIME_LOOPS = 0;
  PUMP_ON_TIME_LOOPS = 0;
  LAST_PUMP_EVENT = 0;
  PUMP_CPH = 0;
  PUMP_GPD = 0;
  LAST_PUMP_EVENT_CUR_TEXT = "";
  LAST_PUMP_EVENT_SS_CUR_TEXT = "";

  // Initialize History
  CycData.init(50, "sec");
  CycData10.init(10, "sec");

  // Initialize Daily History
  GPDDailyHistory.init(20, "GPD");
  CycleDailyHistory.init(20, 2, "min");
  CPHDailyHistory.init(20, 1, "Cyc");
  CurrentDailyHistory.init(20, 2, "Amp");

  // Initialize new 365-day ring
  Daily365.begin();

  // waveform buffers
  curWaveCount = 0;
  lastWaveCount = 0;
  for (int i = 0; i < CUR_WAVE_MAX; i++) {
    curWave[i] = 0;
    lastWave[i] = 0;
  }

  if (TEST_MODE) {
    const int scale = random(1, 11);
  }
}

float PumpData::getAvgCycleMin() { return float(CycData10.avg() / 60.0); }
float PumpData::getStDevCycleMin() { return float(CycData10.stdev() / 60.0); }
float PumpData::getDeltaSec() { return DELTA_SEC; }
float PumpData::getDeltaMin() { return DELTA_MIN; }
float PumpData::getCPH() { return PUMP_CPH; }
int PumpData::getGPD() { return PUMP_GPD; }
int PumpData::getPumpEventCount() { return PUMP_EVENT_COUNT; }
int PumpData::getLastPumpOnTimeLoops() { return LAST_PUMP_ON_TIME_LOOPS; }
uint32_t PumpData::getLastPumpEventLoops() { return LAST_PUMP_EVENT; }
uint32_t PumpData::getPumpOnTimeLoops() { return PUMP_ON_TIME_LOOPS; }

float PumpData::getPumpCur() {
  return 0;
}

int PumpData::getPumpCurCount() { return AVG_PUMP_CUR_COUNT; }

String PumpData::getPumpCurText() {
  return String(0.0, 1) + "A";
}

void PumpData::getPumpCurText(char* out, size_t outSize) {
  if (!out || outSize == 0) return;

  const float amps = (float)getPumpCur();
  snprintf(out, outSize, "%.1fA", amps);
}

String PumpData::getPumpCurTextFull() {
  const float amps = 0.0f;
  return String(amps, 1) + "A [" + String(AVG_PUMP_CUR_COUNT) + "]";
}

String PumpData::getCycleDataText() { return CycData.dataText(); }

// Keep old behavior for now; switch later if you want this to reflect Daily365
int PumpData::getDailyCount() { return GPDDailyHistory.getTotalCount(); }

float PumpData::getLastCycleEnergyAmpSeconds() const {
  const int n = getLastCycleCurrentSamplesCount();
  if (n < 10) return 0.0f;

  const int start = 10;
  const int end = (n - 1 < 49) ? (n - 1) : 49;

  float sumAmps = 0.0f;
  for (int i = start; i <= end; i++) {
    sumAmps += getLastCycleCurrentSampleAmps(i);
  }

  // Samples are at 100ms cadence => 0.1s per sample => divide by 10 to get amp-seconds
  return sumAmps / 10.0f;
}

// -------------------------------------------------------
// TEST_MODE PreLoad
// -------------------------------------------------------


String PumpData::getLastEventCurText() { return LAST_PUMP_EVENT_CUR_TEXT; }
String PumpData::getLastEventSSCurText() { return LAST_PUMP_EVENT_SS_CUR_TEXT; }

// -------------------------
// Waveform accessors
// -------------------------
int PumpData::getLastCycleCurrentSamplesCount() const {
  return lastWaveCount;
}

int PumpData::getLastCycleCurrentSampleCounts(int idx) const {
  if (lastWaveCount <= 0) return 0;
  if (idx < 0) idx = 0;
  if (idx >= lastWaveCount) idx = lastWaveCount - 1;
  return lastWave[idx];
}

float PumpData::getLastCycleCurrentSampleAmps(int idx) const {
  const int c = getLastCycleCurrentSampleCounts(idx);
  return 0.0f;
}

float PumpData::getLastCycleCurrentAvgAmps() const {
  if (lastWaveCount <= 0) return 0.0f;
  uint32_t sum = 0;
  for (int i = 0; i < lastWaveCount; i++) sum += (uint32_t)lastWave[i];
  const float avgCounts = (float)sum / (float)lastWaveCount;
  return 0;
}

float PumpData::getLastCycleCurrentMinAmps() const {
  if (lastWaveCount <= 0) return 0.0f;
  int mn = lastWave[0];
  for (int i = 1; i < lastWaveCount; i++) {
    if (lastWave[i] < mn) mn = lastWave[i];
  }
  return 0.0f;
}

float PumpData::getLastCycleCurrentMaxAmps() const {
  if (lastWaveCount <= 0) return 0.0f;
  int mx = lastWave[0];
  for (int i = 1; i < lastWaveCount; i++) {
    if (lastWave[i] > mx) mx = lastWave[i];
  }
  return 0.0f;
}

String PumpData::getLastCycleCurrentSummaryText() const {
  String s;
  s.reserve(24);
  s += String(lastWaveCount);
  s += " | ";
  s += String(getLastCycleCurrentAvgAmps(), 2);
  s += "A";
  return s;
}

void PumpData::processPumpOnEvent(uint32_t loopCount) {
  PUMP_EVENT_COUNT++;
  DELTA_SEC = (loopCount - LAST_PUMP_EVENT) / LOOPS_PER_SEC;
  DELTA_MIN = DELTA_SEC / 60.0f;
  LAST_PUMP_EVENT = loopCount;

  // Save Event Data for Graph
  eventDataAddNow();

  if (DELTA_SEC > 0) {
    PUMP_CPH = 3600.0f / DELTA_SEC;
  } else {
    PUMP_CPH = 0;
  }

  if (PUMP_EVENT_COUNT > 1) {
    updateAvgCycleTime();
    if (TEST_MODE) {
      updateTestModeHistoryData();
    }
  } else {
    TLog("Ignore first cycle when updating averages");
  }
}

void PumpData::processPumpOffEvent(int pumpOnTimeLoops) {
  //TLog("Pump OFF Event - On Time: %.1f sec", float(pumpOnTimeLoops / 10.0));

  // snapshot current waveform for Details graph
  lastWaveCount = curWaveCount;
  if (lastWaveCount > CUR_WAVE_MAX) lastWaveCount = CUR_WAVE_MAX;

  for (int i = 0; i < lastWaveCount; i++) {
    lastWave[i] = curWave[i];
  }

  // reset for next cycle
  curWaveCount = 0;

  // Capture waveform-based steady-state current (±20% filter)
  float ssCounts = Current.ssAvg(0.2f);
  float ssAmps   = 0.0f;

  LAST_PUMP_EVENT_SS_CUR_TEXT = String(ssAmps, 1) + "A";
  LAST_PUMP_EVENT_CUR_TEXT = Current.dataText();
  LAST_PUMP_ON_TIME_LOOPS = pumpOnTimeLoops;
  PUMP_ON_TIME_LOOPS += pumpOnTimeLoops;
  Current.reset();
}

void PumpData::updateAvgCurrent(int adcCount) {
  int adjCurrent = adcCount;
  int maxCurrent = 0;
  int minCurrent = 0;

  if (adjCurrent > maxCurrent) adjCurrent = maxCurrent;
  if (adjCurrent < minCurrent) adjCurrent = minCurrent;

  if (PUMP_EVENT_COUNT < 3) {
    float newAvgCurrent = 0.90f * AVG_PUMP_CUR_COUNT + 0.10f * adjCurrent;
    AVG_PUMP_CUR_COUNT = int(newAvgCurrent);
  } else {
    float newAvgCurrent = 0.98f * AVG_PUMP_CUR_COUNT + 0.02f * adjCurrent;
    AVG_PUMP_CUR_COUNT = int(newAvgCurrent);
  }

  // capture waveform sample (raw counts) for current cycle
  if (curWaveCount < CUR_WAVE_MAX) {
    curWave[curWaveCount++] = adcCount;
  }

  Current.addData(adcCount);
}

void PumpData::updateAvgCycleTime() {
  CycData.addData(DELTA_SEC);
  CycData10.addData(DELTA_SEC);
}

void PumpData::updateTestModeHistoryData() {
  int gpd = random(100, 300);
  float cph = (gpd / 24.0f) / 5.0f;
  float current = random(900, 1100) / 100.0f;
  GPDDailyHistory.addData(gpd);
  CPHDailyHistory.addData(cph);
  CurrentDailyHistory.addData(current);
  CycleDailyHistory.addData(DELTA_SEC);
}
```

## File: datalogger/pumpData.h
```c
#pragma once
#include "datastore.h"
#include "pump365.h"

#define GAL_PER_MIN 48
#define GAL_PER_CYCLE 5

#define CUR_WAVE_MAX 600

class PumpData {
  public:
    PumpData();
    float getAvgCycleMin();
    float getStDevCycleMin();
    float getDeltaSec();
    float getDeltaMin();
    float getCPH();
    int getGPD();
    int getPumpEventCount();
    int getLastPumpOnTimeLoops();
    uint32_t getPumpOnTimeLoops();
    uint32_t getLastPumpEventLoops();
    float getPumpCur();
    int getPumpCurCount();
    String getPumpCurText();
    void getPumpCurText(char* out, size_t outSize);
    String getPumpCurTextFull();
    String getCycleDataText();
    String getLastEventCurText();
    String getLastEventSSCurText();
    int getDailyCount();

    DataStoreInt GPDDailyHistory;
    DataStoreFloat CycleDailyHistory;
    DataStoreFloat CPHDailyHistory;
    DataStoreFloat CurrentDailyHistory;

    Pump365Data Daily365;

    int   getLastCycleCurrentSamplesCount() const;
    int   getLastCycleCurrentSampleCounts(int idx) const;
    float getLastCycleCurrentSampleAmps(int idx) const;

    float getLastCycleCurrentAvgAmps() const;
    float getLastCycleCurrentMinAmps() const;
    float getLastCycleCurrentMaxAmps() const;

    float getLastCycleEnergyAmpSeconds() const;
    String getLastCycleCurrentSummaryText() const;

    void processPumpOnEvent(uint32_t loopCount);
    void processPumpOffEvent(int pumpOnTimeLoops);
    void updateAvgCurrent(int adcCount);

  private:
    uint32_t DELTA_SEC;
    float DELTA_MIN;
    int AVG_PUMP_CUR_COUNT;
    int PUMP_EVENT_COUNT;
    int LAST_PUMP_ON_TIME_LOOPS;
    uint32_t PUMP_ON_TIME_LOOPS;
    uint32_t LAST_PUMP_EVENT;
    float PUMP_CPH;
    int PUMP_GPD;
    String LAST_PUMP_EVENT_CUR_TEXT;
    String LAST_PUMP_EVENT_SS_CUR_TEXT;

    DataStoreInt CycData;
    DataStoreInt CycData10;
    CurrentStoreInt Current;

    int curWave[CUR_WAVE_MAX];
    int curWaveCount;
    int lastWave[CUR_WAVE_MAX];
    int lastWaveCount;

    void updateAvgCycleTime();
    void updateTestModeHistoryData();
};

// Pump Instance
extern PumpData Pump;
```

## File: datalogger/pumpFunc.cpp
```cpp
// Libraries
#include <event.h>

// Local files
#include "global.h"
#include "ledFunc.h"
#include "ntp.h"
#include "nvm.h"
#include "pumpData.h"
#include "pumpFunc.h"
#include "test_mode.h"
#include "wifiFunc.h"  // timeValid()


// Pump Object
PumpData Pump;

// Events
Event simEvent;

// PUMP Variables
int PUMP_EVENT = FALSE;
long AVG_COUNT = 0;
uint32_t PUMP_EVENT_ON_TIME_LOOPS = 0;
int cycleLockoutCount = 0;

// History
uint32_t GAL;
uint32_t CYCLE;
uint32_t GAL_TODAY = 0;
uint32_t CYCLE_TODAY = 0;

bool gYesterdayWasZero = false;
char blockDayKey[16] = {0};


// Methods
void processPumpEvent() {
  // Check for new pump event
  if (PUMP_EVENT && !cycleLockoutCount) {
    Pump.processPumpOnEvent(LOOP_COUNT);
    Serial.println("[PUMP] ON");

    cycleLockoutCount = LOCKOUT_TIME_LOOPS;
    LEDOn();
  }

  // Clear Event
  PUMP_EVENT = 0;

  // Check for lockout timeout
  if (cycleLockoutCount == 1) {
    LEDOff();
    int onLoops = (int)PUMP_EVENT_ON_TIME_LOOPS;
    if (onLoops > 10) onLoops -= 1;   // Compensate off threshold-read latency of one extra count
    Pump.processPumpOffEvent(onLoops);
    int cycles = Pump.getPumpEventCount();
    float monitorMin = float(LOOP_COUNT / 60.0 / LOOPS_PER_SEC);
    float lastOnTime = float(Pump.getLastPumpOnTimeLoops()) / 10.0f;
    Serial.printf("\n[PUMP] Event: %d | ts: %.1f min | onTime: %.1f sec\n",
      cycles, monitorMin, lastOnTime);

    cycleLockoutCount = 0;
    PUMP_EVENT_ON_TIME_LOOPS = 0;
  } 
  else if (cycleLockoutCount > 1) cycleLockoutCount--;
}

void checkForPumpEvent() {
  int offLevel = int(4 * 0);
  // compute adapative off level after the inrush period
  if (Pump.getPumpEventCount() > 10) offLevel = int(Pump.getPumpCurCount() * 0.80);

  // Only consider on time above the 80 % threshold
  if (ADC1_COUNT > offLevel) {
    PUMP_EVENT = TRUE;
    PUMP_EVENT_ON_TIME_LOOPS++;
    VLog("Event Detected");
  }
}

void initPump() {
  if (TEST_MODE) {
    initPumpSim();
    simEvent.setMin(3);  // First write after 3 mins in test mode
  } 
}

static void persistStateToNvm(uint8_t idx)
{
  if (!timeValid()) return;

  char dayKey[16]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));

  nvmSaveState(
    dayKey,
    nowEpoch,
    GAL, 
    CYCLE
  );
}

void clearDailyPumpData() {
  GAL = 0;
  CYCLE = 0;

  GAL_TODAY = 0;
  CYCLE_TODAY = 0;
  blockDayKey[0] = '\0';
}
```

## File: datalogger/pumpFunc.h
```c
#pragma once

#define BLOCK_DAY_KEY_SIZE 16

// external pump variables
extern int PUMP_EVENT;
extern float PUMP_CPH;
extern int PUMP_GPD;
extern int cycleLockoutCount;
extern uint32_t GAL_TODAY;
extern uint32_t CYCLE_TODAY;
extern uint32_t GAL;
extern uint32_t CYCLE;
extern bool gYesterdayWasZero;
extern char blockDayKey[BLOCK_DAY_KEY_SIZE];

// getters


// setters

// Pump Defines
#define LOCKOUT_TIME_LOOPS 10 * LOOPS_PER_SEC  // 10 sec [No second pump event during this time]
#define TEST_NEXT_EVT_MIN_SEC 50   
#define TEST_NEXT_EVT_MAX_SEC 70   

// Pump Functions
void processPumpEvent();
void checkForPumpEvent();
void initPump();
```

## File: datalogger/ramlog.cpp
```cpp
#include "ramlog.h"
#include "global.h"

extern char* getTimestamp();

char gLogMsg[LOG_LINES][LOG_LEN] = {{0}};
char gLogTs[LOG_LINES][TS_LEN] = {{0}};
uint8_t gLogIndex = 0;
uint8_t gLogCount = 0;

void ramLog(const char* msg) {
  const char* ts = getTimestamp();
  if (!ts) ts = "?";

  snprintf(gLogTs[gLogIndex], sizeof(gLogTs[gLogIndex]), "%s", ts);
  snprintf(gLogMsg[gLogIndex], sizeof(gLogMsg[gLogIndex]), "%s", msg ? msg : "");

  gLogIndex = (gLogIndex + 1) % LOG_LINES;
  if (gLogCount < LOG_LINES) gLogCount++;
}

void ramLogf(const char* fmt, ...) {
  char msg[64];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  const char* ts = getTimestamp();
  if (!ts) ts = "?";

  snprintf(gLogTs[gLogIndex], sizeof(gLogTs[gLogIndex]), "%s", ts);
  snprintf(gLogMsg[gLogIndex], sizeof(gLogMsg[gLogIndex]), "%s", msg);

  gLogIndex = (gLogIndex + 1) % LOG_LINES;
  if (gLogCount < LOG_LINES) gLogCount++;
}
```

## File: datalogger/ramLog.h
```c
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
```

## File: datalogger/server.cpp
```cpp
#include "global.h"

#include "charts.h"
#include "diag.h"
#include "export.h"
#include "pumpData.h"
#include "ntp.h"
#include "nvm.h"
#include "pumpFunc.h"
#include "utils.h"

extern uint16_t ADAPTIVE_DELAY;

// Create Wifi Server
WiFiServer server(80);  // Set web server port number to 80

// -------------------------
// Pages 
// -------------------------
static const uint8_t PAGE_PUMP    = 0;
static const uint8_t PAGE_SYSTEMS = 1;
static const uint8_t PAGE_CHARTS  = 2;
static const uint8_t PAGE_CONN    = 3;
static const uint8_t PAGE_BLOCK   = 4;
static const uint8_t PAGE_WIFI    = 7;

// -------------------------
// Constant HTML fragments in flash (saves RAM)
// -------------------------
static const char HTML_HEAD_A[] PROGMEM =
"<!DOCTYPE html><html>"
"<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<link rel=\"icon\" href=\"data:,\">";

static const char HTML_HEAD_SCRIPT[] PROGMEM =
"<script>"
"var curPage='pump';"
"function setPage(p){curPage=p;loadDoc(p);}"
"function loadDoc(p){"
"var xhttp=new XMLHttpRequest();"
"xhttp.onreadystatechange=function(){"
"if(this.readyState==4&&this.status==200){"
"document.getElementById('webpage').innerHTML=this.responseText;"
"}"
"};"
"xhttp.open('GET','/data?p='+encodeURIComponent(p),true);"
"xhttp.send();"
"}"
"function exportPage(p){"
"window.location='/export?p='+encodeURIComponent(p);"
"}"
"function exportJsonPage(p){"
"window.location='/json?p='+encodeURIComponent(p);"
"}"
//"setInterval(function(){loadDoc(curPage);},30000);"
"window.addEventListener('load',function(){loadDoc(curPage);});"
"</script>";

static const bool NAV_LEFT_MODE = true;   // true = left side nav, false = bottom nav

static const char HTML_HEAD_STYLE[] PROGMEM =
"<style>"
":root{"
  "--theme-color:#2F6F73;"
  "--theme-hover:#D9EEF0;"
  "--theme-font-size:18px;"
  "--theme-row-alt:#f5f7f7;"
  "--theme-measurement-color:#2a2a2a;"
  "--nav-left-gap:8px;"
  "--nav-width:110px;"
"}"

// headings
"h1,h2,h3{margin:10px 0;}"

// body
"body{margin:0;}"
"body{text-align:center;font-family:\"Arial\",Arial;font-size:var(--theme-font-size);}"

// Table
"table{width:80%;margin-left:auto;margin-right:auto;}"
"table{border-collapse:collapse;border-spacing:2px;background-color:white;border:4px solid var(--theme-color);}"
"th{padding:10px 14px;background-color:var(--theme-color);color:white;font-size:1.1em;}"
"tr{border:none;}"
"tr:nth-child(even){background-color:var(--theme-row-alt);}"
"tr:last-child{border-bottom:none;}"
"tr:hover{background-color:var(--theme-hover);}"
"td{padding:12px;}"
"td:first-child{font-weight:700;color:var(--theme-measurement-color);}"

// Page Wrap
".page-wrap{text-align:center;}"

// Title Wrap
".title-wrap{width:80%;margin-left:auto;margin-right:auto;text-align:center;}"
"body.nav-left .title-wrap{margin-left:0;margin-right:auto;}"

// Sensor Boxes
".sensor{display:inline-block;padding:6px 14px;border-radius:999px;background-color:var(--theme-color);color:white;font-weight:700;letter-spacing:0.3px;min-width:140px;text-align:center;box-shadow:0 1px 2px rgba(0,0,0,0.15);}"

// Sizes
".small{font-size:0.85em;}"
".large{font-size:1.25em;}"
".huge{font-size:1.5em;font-weight:bold;}"

/* Base nav/button styling */
".nav{display:flex;justify-content:center;gap:12px;flex-wrap:wrap;margin-top:12px;}"
".navbtn{min-width:100px;max-width:160px;flex:0 0 auto;padding:10px 12px;border-radius:10px;border:2px solid var(--theme-color);font-weight:700;font-size:1em;cursor:pointer;box-sizing:border-box;}"
".navbtn:active{transform:scale(0.98);box-shadow:0 1px 3px rgba(0,0,0,0.15);}"
".navbtn.active{background-color:#D9EEF0;color:#2F6F73;border-color:#2F6F73;}"
".navbtn.white{background-color:white;color:var(--theme-color);}"
".navbtn.theme{background-color:var(--theme-color);color:white;}"

/* Bottom nav mode */
"body.nav-bottom .nav{display:flex;justify-content:center;gap:12px;flex-wrap:wrap;margin-top:12px;}"
"body.nav-bottom .navbtn{min-width:100px;max-width:160px;flex:0 0 auto;}"

/* Left nav mode */
"body.nav-left{padding-left:calc(var(--nav-left-gap) + var(--nav-width) + var(--nav-left-gap));}"

"body.nav-left .nav{"
  "display:flex;"
  "flex-direction:column;"
  "align-items:stretch;"
  "position:fixed;"
  "left:8px;"
  "top:70px;"
  "width:110px;"
  "gap:8px;"
  "margin-top:0;"
  "z-index:1000;"
"}"

"body.nav-left .navbtn{"
  "width:100%;"
  "min-width:0;"
  "max-width:none;"
  "padding:8px 6px;"
  "font-size:0.82em;"
  "white-space:normal;"
  "line-height:1.1;"
  "text-align:center;"
"}"

"body.nav-left table{margin-left:0;margin-right:auto;}"

/* chart styling */
".chart-wrap{width:90%;max-width:1000px;margin:16px auto 28px auto;}"
".chart-title{font-weight:700;color:var(--theme-measurement-color);margin:4px 0 8px 0;}"
".chart{width:100%;height:auto;border:2px solid var(--theme-color);border-radius:14px;background:#fff;}"
".chart .grid{stroke:rgba(0,0,0,0.12);stroke-width:1;}"
".chart .line{fill:none;stroke:var(--theme-color);stroke-width:2;}"
".chart .pt{fill:var(--theme-color);} "
".chart text{font-family:Arial;font-size:12px;fill:#2a2a2a;}"

//------------------------------------------------------------------------
/* Phone */
"@media (max-width: 480px){"
  "table{width:95%;}"
  ".title-wrap{width:95%;}"
  ":root{--theme-font-size:16px;}"
  "td{padding:6px;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:4px 10px;min-width:120px;}"

  /* normal bottom mode */
  "body.nav-bottom .nav{gap:6px;}"
  "body.nav-bottom .navbtn{width:30%;min-width:0;max-width:none;padding:6px 6px;font-size:0.8em;border-radius:8px;box-sizing:border-box;white-space:normal;line-height:1.1;}"

  /* force left mode to behave like bottom mode on phone */
  "body.nav-left{padding-left:0;}"

  "body.nav-left .nav{"
    "position:static;"
    "flex-direction:row;"
    "justify-content:center;"
    "flex-wrap:wrap;"
    "width:auto;"
    "gap:6px;"
    "margin-top:12px;"
    "border-right:none;"
    "padding-right:0;"
    "left:auto;"
    "top:auto;"
  "}"

  "body.nav-left .navbtn{"
    "width:30%;"
    "min-width:0;"
    "max-width:none;"
    "padding:6px 6px;"
    "font-size:0.8em;"
    "border-radius:8px;"
    "box-sizing:border-box;"
    "white-space:normal;"
    "line-height:1.1;"
  "}"

  /* restore centered content on phone */
  "body.nav-left table{margin-left:auto;margin-right:auto;}"
  "body.nav-left .chart-wrap{margin-left:auto;margin-right:auto;}"
"}"

//------------------------------------------------------------------------
/* iPad */
"@media (min-width: 481px) and (max-width: 1024px){"
  "table{width:95%;}"
  ".title-wrap{width:95%;}"
  ":root{--theme-font-size:16px;}"
  "td{padding:6px;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:4px 10px;min-width:140px;}"

  "body.nav-bottom .nav{display:flex;justify-content:center;gap:10px;flex-wrap:nowrap;width:100%;max-width:700px;margin-left:auto;margin-right:auto;}"
  "body.nav-bottom .navbtn{flex:1 1 0;min-width:0;padding:8px 4px;font-size:0.9em;border-radius:8px;text-align:center;white-space:nowrap;}"

  "body.nav-left{padding-left:118px;}"
  "body.nav-left .nav{width:98px;left:8px;top:66px;gap:7px;}"
  "body.nav-left .navbtn{font-size:0.78em;padding:8px 5px;border-radius:8px;}"
"}"

//------------------------------------------------------------------------
/* Desktop */
"@media (min-width: 1025px){"
  "table{width:80%;}"
  ":root{--theme-font-size:20px;}"
  "td{padding:4px 8px;line-height:1.8;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:2px 10px;line-height:1.3;min-width:170px;}"

  "body.nav-bottom .nav{display:flex;justify-content:center;gap:14px;flex-wrap:nowrap;width:100%;max-width:920px;margin-left:auto;margin-right:auto;}"
  "body.nav-bottom .navbtn{flex:1 1 0;min-width:0;padding:6px 16px;font-size:1em;border-radius:10px;text-align:center;white-space:nowrap;}"

  "body.nav-left{padding-left:140px;}"
  "body.nav-left .nav{width:116px;left:10px;top:72px;gap:8px;}"
  "body.nav-left .navbtn{font-size:0.85em;padding:8px 6px;border-radius:10px;}"
"}"

"</style>"
"</head><body class=\"";
static const char PUMP_TABLE_OPEN[] PROGMEM =
"<table><tr><th>PUMP INFO</th><th>VALUE</th></tr>";

static const char SYSTEM_TABLE_OPEN[] PROGMEM =
"<table><tr><th>SYSTEM DATA</th><th>VALUE</th></tr>";

static const char CONN_TABLE_OPEN[] PROGMEM =
"<table><tr><th>CONNECTIVITY DATA</th><th>VALUE</th></tr>";

static const char BLOCK_TABLE_OPEN[] PROGMEM =
"<table><tr><th>BLOCK DATA</th><th>VALUE</th></tr>";

static const char WIFI_TABLE_OPEN[] PROGMEM =
"<table><tr><th>WIFI DATA</th><th>VALUE</th></tr>";

static const char HTML_TABLE_CLOSE[] PROGMEM =
"</table>";

static const char HTML_BOTTOM[] PROGMEM =
"</div></body></html>";




// -------------------------
// HTTP helpers
// -------------------------
static inline void httpOkHtml(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println("Cache-Control: no-store");
  client.println();
}

static inline void httpNotFound(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}

static inline void httpCsvAttachment(WiFiClient &client, const char *filename) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/csv");
  client.print("Content-Disposition: attachment; filename=\"");
  client.print(filename);
  client.println("\"");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
}

static inline void httpJsonAttachment(WiFiClient &client, const char *filename) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.print("Content-Disposition: attachment; filename=\"");
  client.print(filename);
  client.println("\"");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
}

// -------------------------
// Table helpers (streamed, minimal Strings)
// -------------------------
static inline void printRowOpen(WiFiClient &client, const __FlashStringHelper *label) {
  client.print(F("<tr><td>"));
  client.print(label);
  client.print(F("</td><td><span class=\"sensor\">"));
}

static inline void printRowClose(WiFiClient &client) {
  client.println(F("</span></td></tr>"));
}

template<typename T>
static inline void printRow(WiFiClient &client, const __FlashStringHelper *label, T value) {
  printRowOpen(client, label);
  client.print(value);
  printRowClose(client);
}

static inline void printRow(WiFiClient &client, const __FlashStringHelper *label, uint8_t value) {
  printRowOpen(client, label);
  client.print((unsigned int)value);  // force numeric
  printRowClose(client);
}

// Reads first request line without allocating big Strings; returns true if got a line.
static bool readRequestLine(WiFiClient &client, char *buf, size_t buflen, uint32_t timeoutMs = 1500) {
  uint32_t start = millis();
  size_t idx = 0;

  while (millis() - start < timeoutMs) {
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\n') {
        buf[idx] = '\0';
        return true;
      }
      if (c != '\r') {
        if (idx + 1 < buflen) buf[idx++] = c;
      }
    }
    delay(1);
  }
  buf[idx] = '\0';
  return idx > 0;
}

// Extract "p=" page parameter from path (supports /data?p=... and /export?p=...)
// Defaults to Pump.
static uint8_t parsePageParam(const char *path, size_t pathLen) {
  const char *q = (const char*)memchr(path, '?', pathLen);
  if (!q) return PAGE_PUMP;

  const char *p = strstr(q, "p=");
  if (!p) return PAGE_PUMP;
  p += 2;

  if (strncmp(p, "systems", 7) == 0) return PAGE_SYSTEMS;
  if (strncmp(p, "charts", 6) == 0)  return PAGE_CHARTS;
  if (strncmp(p, "conn", 4) == 0)    return PAGE_CONN;
  if (strncmp(p, "block", 5) == 0)   return PAGE_BLOCK;
  if (strncmp(p, "wifi", 4) == 0)    return PAGE_WIFI; 

  return PAGE_PUMP;
}

// Navigation Buttons 
// PUMP / SYS / CON / BLOCK / WIFI / CHART / EXPORT* / JSON* / REFERSH*)
static void renderNavButtons(WiFiClient &client, uint8_t active) {
  client.println(F("<div class=\"nav\">"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_PUMP ? "active" : "white");
  client.println(F("\" onclick=\"setPage('pump')\">PUMP</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_SYSTEMS ? "active" : "white");
  client.println(F("\" onclick=\"setPage('systems')\">SYS</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_CONN ? "active" : "white");
  client.println(F("\" onclick=\"setPage('conn')\">CONN</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_BLOCK ? "active" : "white");
  client.println(F("\" onclick=\"setPage('block')\">BLOCK</button>"));


  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_WIFI ? "active" : "white");
  client.println(F("\" onclick=\"setPage('wifi')\">WIFI</button>"));

  client.println(F("<button class=\"navbtn theme\" onclick=\"setPage('charts')\">CHART</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"exportPage(curPage)\">EXPORT</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"exportJsonPage(curPage)\">JSON</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"loadDoc(curPage)\">REFRESH</button>"));

  client.println(F("</div>"));
}

// -------------------------
// Renderers (dynamic /data)
// -------------------------
static void renderPumpTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)PUMP_TABLE_OPEN);

  const bool noNewDataYetToday = (CYCLE_TODAY == 0);
  const bool showNA = (gYesterdayWasZero && noNewDataYetToday);

  // Total Cycles
  printRow(client, F("Pump Cycles"), String(Pump.getPumpEventCount()));

    // Cycles per HR [Day] (hold over unless yesterday was zero)
  if (showNA) {
    printRow(client, F("Cycles per HR [Day]"), F("N/A"));
  } else if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Cycles per HR [Day]"), F("***"));
  } else {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;
    const float cph = 60.0f / deltaMin;
    const int cpd = (int)(60.0f * 25.0f / deltaMin);

    String s;
    s.reserve(24);
    s = String(cph, 2);
    s += " [";
    s += String(cpd);
    s += "]";
    printRow(client, F("Cycles per HR [Day]"), s);
  }

  // Gallons per Hour [Day] (hold over unless yesterday was zero)
  // Note using 5 Gallons per Cycle (Probably should make this consistent)
  if (showNA) {
    printRow(client, F("Gallons per Hour [Day]"), F("N/A"));
  } else if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Gallons per Hour [Day]"), F("***"));
  } else {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;
    const int  gph = (int)(5.0f * (60.0f / deltaMin));
    const uint32_t gpd = (uint32_t)(5.0f * 60.0f * 24.0f / deltaMin);

    String s;
    s.reserve(28);
    s = String(gph);
    s += " GPH [";
    s += String(gpd);
    s += "]";
    printRow(client, F("Gallons per Hour [Day]"), s);
  }

  const float loopsPerSec = (float)LOOPS_PER_SEC;

  // Total pump ON time
  const float onMin  = (float)Pump.getPumpOnTimeLoops() / (60.0f * loopsPerSec);
  const float onHour = (float)Pump.getPumpOnTimeLoops() / (3600.0f * loopsPerSec);
  if (onMin < 60.0f) printRow(client, F("TOTAL Pump ON Time"), String(onMin, 1) + "m");
  else               printRow(client, F("TOTAL Pump ON Time"), String(onHour, 2) + "hr");

  // Last pump ON time (seconds)
  const float lastOnSec = (float)Pump.getLastPumpOnTimeLoops() / loopsPerSec;
  printRow(client, F("Last Pump ON Time"), String(lastOnSec, 1) + "s");

  // Last cycle time
  if (Pump.getPumpEventCount() < 2) printRow(client, F("Last Cycle Time"), F("***"));
  else                              printRow(client, F("Last Cycle Time"), String(Pump.getDeltaMin(), 2) + "m");

  // Time since last cycle
  if (Pump.getPumpEventCount() < 1) {
    printRow(client, F("Time Since Last Cycle"), F("***"));
  } else {
    const float offTimeMin = (float)(LOOP_COUNT - Pump.getLastPumpEventLoops()) / (60.0f * loopsPerSec);
    printRow(client, F("Time Since Last Cycle"), String(offTimeMin, 2) + "m");
  }

  // Avg cycle + stdev
  if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Avg Cycle Time [StDev]"), F("***"));
  } else {
    String s;
    s.reserve(40);
    s = String(Pump.getAvgCycleMin(), 2);
    s += "m [";
    s += String(Pump.getStDevCycleMin(), 2);
    s += "]";
    printRow(client, F("Avg Cycle Time [StDev]"), s);
  } 

  // Current (steady-state from last completed event) + Energy in brackets
  if (Pump.getPumpEventCount() < 1) {
    printRow(client, F("Pump Current [Energy]"), F("***"));
  } else {
    String s;
    s.reserve(32);
    s = Pump.getLastEventSSCurText();               // e.g. "9.3A"
    s += " [";
    s += String(Pump.getLastCycleEnergyAmpSeconds(), 1);  // e.g. "142.6"
    s += " A·s]";

    printRow(client, F("Pump Current [Energy]"), s);
  }

  // Wifi Status
  printRow(client, F("Wifi Status"), CONN_STATUS);


  // Timestamp
  printRow(client, F("Timestamp"), getTimestamp());

  // Monitor time
  const float monMin = (float)LOOP_COUNT / (60.0f * loopsPerSec);
  const float monHr  = (float)LOOP_COUNT / (3600.0f * loopsPerSec);
  const float monDay = (float)LOOP_COUNT / (86400.0f * loopsPerSec);

  if (monHr < 1.0f)       printRow(client, F("Total Monitor Time"), String(monMin, 1) + "m");
  else if (monDay < 1.0f) printRow(client, F("Total Monitor Time"), String(monHr, 2) + "hr");
  else                    printRow(client, F("Total Monitor Time"), String(monDay, 2) + "d");

  client.print((const __FlashStringHelper*)HTML_TABLE_CLOSE);
  renderNavButtons(client, PAGE_PUMP);
}

static void renderSystemTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)SYSTEM_TABLE_OPEN);

  printRow(client, F("Loop Count (100ms)"), String(LOOP_COUNT));
  printRow(client, F("Last-cycle Current (n | avg)"), Pump.getLastCycleCurrentSummaryText());
  printRow(client, F("Last Cycle Energy"), String(Pump.getLastCycleEnergyAmpSeconds(), 2) + " A·s");


  printRow(client, F("1 Sec Run Time"), String(LOOP_TIME) + " ms");
  printRow(client, F("Adaptive Loop Delay"), String(ADAPTIVE_DELAY) + " ms");
  printRow(client, F("Daily Totals Stored"), String(Pump.Daily365.dailyValidCount()) + " days");

  client.print((const __FlashStringHelper*)HTML_TABLE_CLOSE);

  // Nav Buttons
  renderNavButtons(client, PAGE_SYSTEMS);
}


static void renderConnTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)CONN_TABLE_OPEN);

  // Nav Buttons
  renderNavButtons(client, PAGE_CONN);
}

static void renderBlockTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)BLOCK_TABLE_OPEN);

  printRow(client, F("Last Block Day"), blockDayKey);
  printRow(client, F("Total Block Writes"), getTotalBlockWriteCount());

  // block data
  printRow(client, F("Cycle"), CYCLE);
  printRow(client, F("Gallons"), GAL);

  // Nav Buttons
  renderNavButtons(client, PAGE_BLOCK);
}



static void renderWifiTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)WIFI_TABLE_OPEN);

  printRow(client, F("WiFi State"), diagState.getWifiState());
  printRow(client, F("WiFi RSSI"), diagState.getWifiRSSI());
  printRow(client, F("WiFi BSSID"), diagState.getWifiBSSID());
  printRow(client, F("WiFi GW"), diagState.getWifiGW());
  printRow(client, F("WiFi IP"), diagState.getWifiIP());
  printRow(client, F("WiFi DNS"), diagState.getWifiDNS());
  printRow(client, F("WiFi Errors"), String(WIFI_ERR));

  // Nav Buttons
  renderNavButtons(client, PAGE_WIFI);
}

static void renderChartsPage(WiFiClient &client) {

  client.println(F("<h3>Charts</h3>"));
  // Charts
  renderGallonsPerDayChart(client);
  renderCurrentWaveChart(client);
  renderPumpEvent24hChart(client);

  // Nav Buttons
  renderNavButtons(client, PAGE_CHARTS);
}

static void renderData(WiFiClient &client, uint8_t page) {
  switch (page) {
    case PAGE_SYSTEMS: renderSystemTable(client);  break;
    case PAGE_CHARTS:  renderChartsPage(client);   break;
    case PAGE_CONN:    renderConnTable(client);    break;
    case PAGE_BLOCK:   renderBlockTable(client);   break;
    case PAGE_WIFI:    renderWifiTable(client);    break;
    case PAGE_PUMP:
    default:           renderPumpTable(client);    break;
  }
}


// -------------------------
// Shell (served at "/")
// -------------------------
static void renderPageShell(WiFiClient &client) {
  client.print((const __FlashStringHelper*)HTML_HEAD_A);
  client.print((const __FlashStringHelper*)HTML_HEAD_SCRIPT);
  client.print((const __FlashStringHelper*)HTML_HEAD_STYLE);
  client.print(NAV_LEFT_MODE ? "nav-left" : "nav-bottom");
  client.println(F("\">"));

  client.println(F("<div class=\"page-wrap\">"));
  client.println(F("<div class=\"title-wrap\">"));
  client.print(F("<h3>Shahman Data Logger "));
  client.print(APP_VERSION);
  if (TEST_MODE) client.print(F(" *TEST*"));
  client.println(F("</h3>"));
  client.println(F("</div>"));

  client.println(F("<div id=\"webpage\"></div>"));

  client.println(F("</div>"));

  client.print((const __FlashStringHelper*)HTML_BOTTOM);
}

// -------------------------
// Main entry
// -------------------------
void webServer() {
  WiFiClient client = server.available();
  client.setNoDelay(true);
  if (!client) return;

  char line[180];
  if (!readRequestLine(client, line, sizeof(line))) {
    client.stop();
    return;
  }

  const bool isGet = (strncmp(line, "GET ", 4) == 0);
  const char *path = isGet ? (line + 4) : nullptr;

  bool isRoot = false;
  bool isData = false;
  bool isExport = false;
  bool isJson = false;
  uint8_t page = PAGE_PUMP;

  if (path && path[0] == '/') {
    const char *sp = strchr(path, ' ');
    size_t len = 0;
    if (sp) len = (size_t)(sp - path);
    else    len = strlen(path);

    isRoot = (len == 1);

    if (len >= 5 && strncmp(path, "/data", 5) == 0) {
      isData = true;
      page = parsePageParam(path, len);
    }

    if (len >= 7 && strncmp(path, "/export", 7) == 0) {
      isExport = true;
      page = parsePageParam(path, len);
    }

    if (len >= 5 && strncmp(path, "/json", 5) == 0) {
      isJson = true;
      page = parsePageParam(path, len);
    }
  }

  if (!isGet || (!isRoot && !isData && !isExport && !isJson)) {
    httpNotFound(client);
    client.stop();
    return;
  }

  if (isExport) {
    char fname[48];
    char ts[32];
    snprintf(ts, sizeof(ts), "%s", getTimestamp());


    // Export Data File
    for (int i = 0; ts[i]; i++) {
      if (ts[i] == ':' || ts[i] == ' ') ts[i] = '_';
    }

    char suffix[10]; 
    if (TEST_MODE) snprintf(suffix, sizeof(suffix), "_test");
    else snprintf(suffix, sizeof(suffix), "");  
    snprintf(fname, sizeof(fname), "pump%s_export_%s.csv", suffix, ts);

    httpCsvAttachment(client, fname);
    renderExportCsv(client);
  } 
  
  // Export JSON
  else if (isJson) {
    char fname[48];
    char ts[32];
    snprintf(ts, sizeof(ts), "%s", getTimestamp());


    // Export JSON File
    for (int i = 0; ts[i]; i++) {
      if (ts[i] == ':' || ts[i] == ' ') ts[i] = '_';
    }

    char suffix[10]; 
    if (TEST_MODE) snprintf(suffix, sizeof(suffix), "_test");
    else snprintf(suffix, sizeof(suffix), "");  
    snprintf(fname, sizeof(fname), "pump%s_json_%s.json", suffix, ts);

    httpJsonAttachment(client, fname);
  } 

  else { 
    httpOkHtml(client);
    if (isData) renderData(client, page);
    else renderPageShell(client);
  }

  //client.flush();
  client.stop();
}
```

## File: datalogger/server.h
```c
#pragma once
void webServer();
```

## File: datalogger/test_mode.cpp
```cpp
// Libraries
#include <event.h>

// Local Files
#include "global.h"
#include "ledFunc.h"
#include "oled.h"

int testModeADC = 0;
int pumpCount = 0;
Event pumpTestEvent;
long testLogCyc = 0;

#define CYCLE_TIME 30
#define PUMP_ON 1
#define PUMP_OFF 0

int getTestModeADC() {
  return testModeADC;
}

void setTestModeADC(float current) {
  //testModeADC = 0;
}

void initPumpSim() {
  pumpTestEvent.setSec(CYCLE_TIME);
}

float getPumpCurrent(int count) {
  int index = 77 - count;
  float cur = 0.0f;
  if (index < 7) {
    cur = 10.0f + 14.0f * expf(-float(index) / 2.5f);
  }
  else if (index < 66) {
    cur = 9.5f + (float)random(0,101) / 100.0f;
  }
  else { // index >= 66
    float tailIndex = (float)(index - 66);      // 0..11
    cur = 10.0f * (1.0f - tailIndex / 11.0f);   // 10 -> 0
    if (cur < 0.0f) cur = 0.0f;
  }
  return cur;
}

void simulatePump() {
  static int simPumpState = PUMP_OFF;

  if (simPumpState == PUMP_OFF) {
    // Look for Turn On Event
    if (pumpTestEvent.check()) {
      // Serial.println("Test Mode Pump Event: ");
      simPumpState = PUMP_ON;
      pumpCount = 77;
    }
    setTestModeADC(0);
  }
  
  else if (simPumpState == PUMP_ON) {
    // Look for Turn Off Event
    if (pumpCount == 0) {
      int nextEvent = random(CYCLE_TIME, CYCLE_TIME+10);
      pumpTestEvent.setSec(nextEvent);
      Serial.println("Set Next Test Mode Pump On Event: " + String(nextEvent) + " sec");
      simPumpState = PUMP_OFF;
      setTestModeADC(0.0f);   // drop to 0 immediately
    }

    else if (pumpCount > 0) {
      setTestModeADC(getPumpCurrent(pumpCount));
      pumpCount--;
    }
  }
  
  else {
    Serial.println("Invalid Simluated Pump State");
  }
}

void simulateLogger() {
  testLogCyc++;
  if (testLogCyc % LOOPS_PER_SEC == 1) {
    toggleLED();
  }
}

void turnOnWifi() {
  newPopupScreen("WIFI ON", "");
}

void turnOffWifi() {
  newPopupScreen("WIFI OFF", "press button to wake");
}
```

## File: datalogger/test_mode.h
```c
#pragma once

int getTestModeADC();
void simulatePump();
void initPumpSim();
void simulateLogger();
void turnOnWifi();
void turnOffWifi();
```

## File: datalogger/testcode.cpp
```cpp
// Test Code
#include "global.h"
#include <WiFi.h>
#include "ntp.h"
#include "wifiFunc.h"
```

## File: datalogger/testcode.h
```c
#pragma once
```

## File: datalogger/utils.cpp
```cpp
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
```

## File: datalogger/utils.h
```c
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
```

## File: datalogger/wifiFunc.cpp
```cpp
// wifiFunctions.cpp
#include "global.h"

#include "diag.h"
#include "WiFi.h"
#include "wifiFunc.h"
#include "oled.h"
#include "ledFunc.h"
#include "secrets.h"
#include <lwip/inet.h>   // for INADDR_NONE (ESP32)
#include <time.h>
#include <WiFiClientSecure.h>

#define TEST_DHCP_ONLY 0   // set to 0 to restore DHCP->static reconnect

// external variables
extern const char* CONN_STATUS;
extern WiFiServer server;

static bool serverStarted = false;
uint8_t wifiRadioState = 0;

bool wifiLinkUp() {
  return WiFi.status() == WL_CONNECTED &&
         WiFi.localIP() != IPAddress(0,0,0,0) &&
         WiFi.dnsIP()  != IPAddress(0,0,0,0);
}

bool timeValid() {
  return time(nullptr) > 1577836800; // 2020-01-01
}

bool syncTime(uint32_t timeoutMs = 15000) {
  Serial.println("Syncing time (NTP)...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    time_t now = time(nullptr);
    if (now > 1700000000) {
      Serial.print("Epoch time: ");
      Serial.println((uint32_t)now);
      return true;
    }
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nNTP sync FAILED");
  Serial.print("Epoch time: ");
  Serial.println((uint32_t)time(nullptr));
  return false;
}

bool wifiLinkReady() {
  static uint32_t goodSinceMs = 0;
  static uint32_t lastBadMs = 0;

  bool connected = (WiFi.status() == WL_CONNECTED);

  if (!connected) {
    goodSinceMs = 0;
    lastBadMs = millis();
    return false;
  }

  IPAddress ip = WiFi.localIP();

  bool ipValid =
    (ip != IPAddress(0,0,0,0)) &&
    (ip != IPAddress(255,255,255,255));

  bool rssiValid = (WiFi.RSSI() < 0);  // RSSI should be negative dBm

  bool good = connected && ipValid && rssiValid;

  if (!good) {
    goodSinceMs = 0;
    lastBadMs = millis();
    return false;
  }

  // Require stability after last "bad"
  if (goodSinceMs == 0) {
    goodSinceMs = millis();
    return false;
  }

  // Require BOTH:
  // - stable for 5s
  // - no disconnects in last 5s
  if ((millis() - goodSinceMs) >= 5000 &&
      (millis() - lastBadMs) >= 5000) {
    return true;
  }

  return false;
}

bool dnsReady() {
  static uint32_t lastCheckMs = 0;
  static bool lastResult = false;

  if (WiFi.status() != WL_CONNECTED) {
    lastResult = false;
    return false;
  }

  if (millis() - lastCheckMs < 5000) {
    return lastResult;
  }

  lastCheckMs = millis();

  IPAddress ip;
  lastResult = (WiFi.hostByName("arduino-27cc5-default-rtdb.firebaseio.com", ip) == 1 &&
                ip != IPAddress(0,0,0,0));
  return lastResult;
}


void updateWifiDiagState() {
  if (WiFi.status() != WL_CONNECTED) {
    diagState.setWifiState("WIFI_DISCONNECTED");
    return;
  }

  IPAddress ip  = WiFi.localIP();
  IPAddress gw  = WiFi.gatewayIP();
  IPAddress dns = WiFi.dnsIP();
  const uint8_t* bssid = WiFi.BSSID();

  char buf[32];

  // BSSID
  if (bssid) {
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
      bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  diagState.setWifiBSSID(buf);

  // RSSI
  snprintf(buf, sizeof(buf),"%d CH:%d", WiFi.RSSI(), WiFi.channel());
  diagState.setWifiRSSI(buf);

  // IP
  snprintf(buf, sizeof(buf),"%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  diagState.setWifiIP(buf);

  // GW
  snprintf(buf, sizeof(buf),"%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
  diagState.setWifiGW(buf);

  // DNS
  snprintf(buf, sizeof(buf),"%u.%u.%u.%u", dns[0], dns[1], dns[2], dns[3]);
  diagState.setWifiDNS(buf);

  // State
  diagState.setWifiState("CONNECTED");
}


void scanWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Skipping WiFi scan: already connected");
        return;
    }

    displayPopupScreen("SCANNING WIFI","Finding Networks");
    updatePopupScreen();
    Serial.println("Starting WiFi scan...");

    int startTime = micros();
    int netCount = WiFi.scanNetworks();
    
    Serial.println("Scan complete");
    if (netCount == 0) {
        Serial.println("NO networks found");
    } else {
        Serial.print(netCount);
        Serial.println(" Networks found");
        for (int i = 0; i < netCount; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");
    Serial.println("Scan time: "+String(micros() - startTime));
}

bool waitForWifiStable(uint32_t stableMs, uint32_t timeoutMs) {
  // Serial.println("Waiting for WiFi to stabilize...");

  uint32_t tStart = millis();
  uint32_t stableStart = 0;

  while (millis() - tStart < timeoutMs) {
    if (wifiLinkUp() && timeValid()) {
      if (stableStart == 0) stableStart = millis();
      if (millis() - stableStart >= stableMs) {
        // Serial.println("WiFi stable.");
        return true;
      }
    } else {
      stableStart = 0; // reset stability window
    }
    delay(50);
  }

  Serial.println("WiFi NOT stable (timeout)");
  return false;
}

bool connectDhcpThenStaticHost(const char* ssid, const char* password) {
    int host = TEST_SERVER_HOST_ID;
    if (!TEST_MODE) host = EDGE_HOST_ID;

  // If already connected with correct static IP, do nothing
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    if (ip[3] == host) {
      return true;
    }
  }

  Serial.println("WiFi: DHCP -> static host reconnect");

  WiFi.disconnect(true);
  delay(250);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  // -----------------------------
  // Step 1: Connect using DHCP
  // -----------------------------
  WiFi.begin(ssid, password);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(200);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi DHCP connect failed");
    return false;
  }

  // Wait until DHCP options are populated
  t0 = millis();
  while ((WiFi.gatewayIP() == IPAddress(0,0,0,0) ||
          WiFi.gatewayIP() == IPAddress(255,255,255,255) ||
          WiFi.subnetMask() == IPAddress(0,0,0,0)) &&
         millis() - t0 < 5000) {
    delay(200);
  }

  IPAddress gw   = WiFi.gatewayIP();
  IPAddress mask = WiFi.subnetMask();
  IPAddress dns  = WiFi.dnsIP();

  if (gw == IPAddress(0,0,0,0) || mask == IPAddress(0,0,0,0)) {
    Serial.println("DHCP did not provide valid network info");
    return false;
  }

#if TEST_DHCP_ONLY
  Serial.println("TEST: staying on DHCP (skipping static reconnect)");
  return true;   // WiFi is connected via DHCP; caller can proceed to NTP
#endif

  // -----------------------------
  // Step 2: Build static IP
  // -----------------------------
  IPAddress staticIP;

  // Common home LAN case (/24)
  if (mask == IPAddress(255,255,255,0)) {
    staticIP = IPAddress(gw[0], gw[1], gw[2], host);
  } else {
    // Fallback: derive network portion
    IPAddress net(gw[0] & mask[0],
                  gw[1] & mask[1],
                  gw[2] & mask[2],
                  gw[3] & mask[3]);

    staticIP = IPAddress(net[0], net[1], net[2], host);

    Serial.print("WARN: non-/24 subnet detected: ");
    Serial.println(mask);
  }

  Serial.print("DHCP IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(gw);
  Serial.print("Subnet: ");
  Serial.println(mask);
  Serial.print("DNS: ");
  Serial.println(dns);
  Serial.print("Static IP: ");
  Serial.println(staticIP);

  // -----------------------------
  // Step 3: Reconnect using static IP
  // -----------------------------
  WiFi.disconnect(true);
  delay(250);

  WiFi.config(staticIP, gw, mask, dns);
  WiFi.begin(ssid, password);

  t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(200);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Static reconnect failed");
    return false;
  }

  Serial.print("WiFi connected with static IP: ");
  Serial.println(WiFi.localIP());
  return (WiFi.localIP() == staticIP);
}

void connectWifi() {

  wifiRadioState = 1;
  char details[64];
  snprintf(details, sizeof(details), "Network: %s", WIFI_SSID);
  displayPopupScreen("CONNECTING...", details);
  updatePopupScreen();

  bool ok = connectDhcpThenStaticHost(WIFI_SSID, WIFI_PSW);

  if (ok) {
    CONN_STATUS = "CONN";
    Serial.println("WiFi connected with static host 186/192");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Keep your time sync here (don’t return early if you want server/UI to keep working)
    if (!syncTime()) {
      Serial.println("NTP failed");
    }

  } else {
    CONN_STATUS = "NOT CONNECTED";
    Serial.println("WiFi connect FAILED");
  }

  oledMain(MAIN_TIMEOUT_SEC);
  LEDOff();
}

void disconnectWifi() {
  wifiRadioState = 0;
  CONN_STATUS = "NOT_CONNECTED";
  WiFi.disconnect(true);   // Disconnect and stop reconnect attempts
  WiFi.mode(WIFI_OFF);     // Turn off the Wi-Fi radio
}

bool wifiRadioOn() {
  return wifiRadioState == 1;
}

bool ensureServerStarted() {
  // If WiFi dropped, allow restart later
  if (serverStarted && WiFi.status() != WL_CONNECTED) {
    serverStarted = false;
  }

  // Only start once we have a real IP
  IPAddress ip = WiFi.localIP();
  bool hasIP = (ip != IPAddress(0,0,0,0) && ip != IPAddress(255,255,255,255));

  if (!serverStarted && WiFi.status() == WL_CONNECTED && hasIP) {
    server.begin();
    serverStarted = true;
    Serial.println("SERVER STARTED");
    Serial.print("Open: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }

  return serverStarted;
}


bool wifiOK() {
    if (WiFi.status() == WL_CONNECTED  &&
        WiFi.localIP() != IPAddress(0,0,0,0)) {
    CONN_STATUS = "CONN";
    return true;
    }

    CONN_STATUS = "OFF";
    return false;
}

bool dnsOK() {
  return WiFi.dnsIP() != IPAddress(0,0,0,0);
}

bool internetOK() {
  WiFiClient c;
  int timeout = 2000;
  c.setTimeout(timeout / 1000);
  return c.connect("1.1.1.1", 80);
}

bool internetOK443() {
  WiFiClientSecure c;
  int timeout = 3000;
  c.setInsecure();                 // reachability test only
  c.setTimeout(timeout / 1000);    // seconds
  bool ok = c.connect("www.google.com", 443);
  c.stop();
  return ok;
}
```

## File: datalogger/wifiFunc.h
```c
#pragma once

void scanWifi();
void connectWifi();
void disconnectWifi();
bool ensureServerStarted();
bool waitForWifiStable(uint32_t stableMs = 1500, uint32_t timeoutMs = 20000);
bool wifiOK();
bool dnsOK();
bool internetOK();
bool internetOK443();
bool timeValid();
void updateWifiDiagState();

// Helpers
bool dnsReady();
bool wifiLinkReady();
bool wifiRadioOn();

#define EDGE_HOST_ID 186
#define TEST_SERVER_HOST_ID 192
// #define SUBNET_ID 50
#define USE_STATIC_IP 0
```

## File: README-datalogger.md
```markdown
# Data Logger (ESP32 + OLED + WiFi + Firebase)

An ESP32-based data logger with OLED status screens, WiFi
connectivity, NTP time sync.

This system reads IO and logs to NVM and provides statsusd reports to a
web server. It also includes a built-in web UI, OTA
firmware update support.

------------------------------------------------------------------------

## Features

-   OLED UI (Heltec SSD1306)
-   WiFi STA with DHCP → static-IP reconnect model
-   NTP time synchronization (fixed UTC offset, no DST)
-   NVS-backed data
-   Boot statistics persistence (reboot counter + timestamps)
-   ArduinoOTA firmware update support
-   Embedded Web UI (Multi Function Pages / Export / Charts)
-   CSV / JSON exports
-   Pixel Shifting to save OLED screen from burn in (learned the hard way)
-   Watchdog Timer
-   Serial Logger / RAM Logger (for Prod)
-   Test Mode 
-   Adaptive loop delay tuning

------------------------------------------------------------------------

## Hardware

Typical setup:

-   ESP32 (Heltec or compatible)
-   SSD1306 128×64 OLED
-   ADC input
-   LED indicator
-   touch/button input

GPIO and configuration constants are defined in `global.h`.

------------------------------------------------------------------------

## System Overview (Conceptual Flow)

** NEED TO REWRITE **

Pump Current Signal\
↓\
Cycle Detection\
↓\
Runtime Accumulation\
↓\
4-Hour Block Aggregation\
↓\
NVM Checkpoint\
↓\
Reboot? → Restore + Continue\
↓\
Daily Summary (23:00)\
↓\
Firebase Write (if safe)\
↓\
365-Day History → Web UI

------------------------------------------------------------------------

## Scheduling Model (NEEDS UPDATE)

Main loop runs every \~100ms.

Soft task structure:

-   `loop100ms()` --- input sampling, pump detection, OLED updates
-   `loop1Sec()` --- adaptive delay control, NTP updates, OTA servicing
-   `loop10Sec()` --- server start + connection checks
-   `loop1Min()` --- WiFi/Firebase reconnect logic
-   `loop1Hour()` --- Firebase summary retry
-   `loop1Day()` --- reset daily write counter

OTA servicing may also be called in the main loop to ensure responsive
firmware updates.

------------------------------------------------------------------------

## CONNECTIVITY:

System     Reliability
---------- ------------------------------
NTP        HTTP: Always works

------------------------------------------------------------------------

## Daily Lifecycle

** NEED TO REWRITE **

The system operates on a repeating daily cycle.

Each day is divided into 6 blocks:

-   03:00 / 07:00 / 11:00 / 15:00 / 19:00 / 23:00

At each boundary:

-   the previous 4 hours are finalized and reset
-   data is stored / checkpointed
-   state is persisted

At 23:00:

-   the full day is known
-   totals are computed
-   estimates are computed if needed
-   Firebase is queued, and queue store in NVM

------------------------------------------------------------------------

------------------------------------------------------------------------

## Firebase Architecture

### Write Model

The system performs:

-   6 × 4-hour model buckets per day (3,7,11,15,19,23)
-   One authoritative daily summary write

Intermediate 4-hour bucket state is persisted to NVS to prevent data
loss during firmware updates or unexpected reboots.

### Paths

Normal mode:

    /pump/<namespace>/<year>/daily_summary/YYYY_MM_DD/
    /pump/<namespace>/<year>/daily_detail/YYYY_MM_DD/

Test mode:

    /pump/<namespace>/test_server/<year>/daily_summary/YYYY_MM_DD/
    /pump/<namespace>/test_server/<year>/daily_detail/YYYY_MM_DD/

### Daily Summary Stucture (NEEDS UPDATE)

-   timestamp
-   total_gallons (model-derived)
-   total_cycles (model-derived)
-   cycle1..cycle6
-   gallon1..gallon6
-   last_cycle_energy (Amp-seconds proxy)



------------------------------------------------------------------------

## NVM Checkpointing  (UPDATE)

### 4-Hour Block Persistence

After each completed 4-hour block these items are written to NVS:

-   `GAL_4HR[6]`
-   `CYCLE_4HR[6]`
-   `daily4hrCount`
-   `day_key`
-   `save_epoch`

On reboot:

-   State is restored only if:
    -   Day key matches current day
    -   Saved timestamp is within configured window (4 hours)

If validation fails, state is ignored and cleared.

### Boot Statistics

On each reboot:

-   Boot counter increments
-   Previous boot timestamp stored
-   Last boot timestamp stored



------------------------------------------------------------------------

## Web UI (NEEDS UPDATE)

Accessible at device IP address.

Pages:

-   MAIN --- real-time stats
-   TREND --- daily history + gallons/day SVG chart
-   DETAILS --- waveform graph + energy metrics

------------------------------------------------------------------------

## Charts

Charts are rendered as inline SVG (no external JS libraries).

1) Gallons Per Day (last 365 days
2) Current Waveform from last pump on event
3) On / Off pulse wave for last 24 hours


------------------------------------------------------------------------

## OTA Firmware Update

Supports ArduinoOTA-based firmware updates over WiFi.

-   Requires OTA-enabled firmware build
-   Uses configurable TCP port (default 3232)
-   Requires OTA handler to be serviced in main loop
-   First upload must be performed via USB

------------------------------------------------------------------------

## Stability Improvements

-   Boot statistics tracking
-   Reduced dynamic memory in OLED hot path
-   Static HTML fragments stored in PROGMEM
-   Adaptive loop delay tuning

------------------------------------------------------------------------

# Time Handling

-   Real Time clock synced with NTP time server
-   Automatic time zone with DST handling
-   Time zone sanity checking every minute

------------------------------------------------------------------------

## Test Mode

Test mode simulates:

-  Data to be logged

Used for UI validation without hardware.

------------------------------------------------------------------------

## Prod Diagnostics (No Serial Available)

### RAM Log

-   circular buffer (\~100 lines)
-   timestamps
-   exportable

### CSV Export

Exports as much internal data / variables as possible:

** NEED TO UPDATE **
-   system states
-   Firebase state
-   Wifi State
-   NVM state
-   RAM log


This is the **primary debugging interface**.

------------------------------------------------------------------------

## Test Mode Diagnostics

Test mode supports very detailed serial logging

------------------------------------------------------------------------

## Build / Flash

1.  Install Arduino IDE or PlatformIO
2.  Install ESP32 board support
3.  Configure `secrets.h` (WiFi + Firebase)
4.  Upload firmware (USB required for first OTA-enabled build)

------------------------------------------------------------------------

## Project Structure (NEEDS UPDATE)

Main File - Pump_Monitor.ino

Pump detection + metrics: Pump_Monitor.cpp / pumpFunc.cpp
Data: pumpData.cpp / datastore.cpp
WiFi + Time: wifiFunctions.cpp / ntp.cpp
UI + Indicators - oledFunc.cpp / ledFunc.cpp
Web Server: serverFunc.cpp
OTA: OTA.cpp
NVM: nvm.cpp
Test Mode: test_mode.cpp
Charts: charts.cpp, pump365.h, eventData.cpp
Diagnostics: diag.cpp
Export Generation: export.cpp
Utilities: utils.cpp


Config: global.h
Secrets: secrets.h

------------------------------------------------------------------------

## License

Private project.

------------------------------------------------------------------------

## Limitations




## Future Improvements

- 
- Encapsulate globals and diagnostics
- Move Daily Statistics to NVM
- Move RAM log to web page
- Add an NVM log for crash debugging
```

## File: repomix.config.json
```json
{
  "$schema": "https://repomix.com/schemas/latest/schema.json",
  "input": {
    "maxFileSize": 52428800
  },
  "output": {
    "filePath": "repomix-datalogger.md",
    "style": "markdown",
    "parsableStyle": false,
    "fileSummary": true,
    "directoryStructure": true,
    "files": true,
    "removeComments": false,
    "removeEmptyLines": false,
    "compress": false,
    "topFilesLength": 5,
    "showLineNumbers": false,
    "truncateBase64": false,
    "copyToClipboard": false,
    "includeFullDirectoryStructure": false,
    "tokenCountTree": false,
    "git": {
      "sortByChanges": true,
      "sortByChangesMaxCommits": 100,
      "includeDiffs": false,
      "includeLogs": false,
      "includeLogsCount": 50
    }
  },
  "include": [],
  "ignore": {
    "useGitignore": true,
    "useDotIgnore": true,
    "useDefaultPatterns": true,
    "customPatterns": []
  },
  "security": {
    "enableSecurityCheck": true
  },
  "tokenCount": {
    "encoding": "o200k_base"
  }
}
```
