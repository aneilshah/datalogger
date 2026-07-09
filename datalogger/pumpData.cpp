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