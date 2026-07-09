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
Event sim4HrEvent;

// PUMP Variables
int PUMP_EVENT = FALSE;
long AVG_COUNT = 0;
uint32_t PUMP_EVENT_ON_TIME_LOOPS = 0;
int cycleLockoutCount = 0;

// History
uint32_t GAL_4HR[NUM_4HR_BLOCKS];
uint32_t CYCLE_4HR[NUM_4HR_BLOCKS];
uint32_t GAL_TODAY = 0;
uint32_t CYCLE_TODAY = 0;
uint8_t DATA_TYPE = DATA_SYSTEM;
bool gYesterdayWasZero = false;
char blockDayKey[16] = {0};

// Getters
uint8_t getDataType() {return DATA_TYPE;}

// Setters
void setDataType(uint8_t data_type) {DATA_TYPE = data_type;}

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
    sim4HrEvent.setMin(3);  // First write after 3 mins in test mode
  } 
}

static void persist4HrStateToNvm(uint8_t idx)
{
  if (!timeValid()) return;

  char dayKey[16]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));

  nvmSave4hrState(
    dayKey,
    nowEpoch,
    idx,
    (uint32_t*)GAL_4HR, NUM_4HR_BLOCKS,
    (uint32_t*)CYCLE_4HR, NUM_4HR_BLOCKS
  );
}

void clearDailyPumpData() {
  for (int n = 0; n < NUM_4HR_BLOCKS; n++) {
    GAL_4HR[n] = 0;
    CYCLE_4HR[n] = 0;
  }
  GAL_TODAY = 0;
  CYCLE_TODAY = 0;
  setDataType(DATA_SYSTEM);
  blockDayKey[0] = '\0';
}

void processFBWriteEvent() {
  static int8_t lastHour = -1;
  uint8_t hour = getHourInt();

  // --- 4HR measured delta snapshots ---
  static uint64_t last4HrPumpOnLoops = 0;
  static uint64_t last4HrPumpEventCount = 0;
  static bool snapshotsInit = false;

  if (!snapshotsInit) {
    last4HrPumpOnLoops     = (uint64_t)Pump.getPumpOnTimeLoops();
    last4HrPumpEventCount  = (uint64_t)Pump.getPumpEventCount();
    snapshotsInit = true;
  }

  // -----------------------
  // TEST MODE
  // -----------------------
  if (TEST_MODE) {

    if (sim4HrEvent.check()) {
      // Snapshot current cumulative counters ONLY at the block boundary
      uint64_t curOnLoops = (uint64_t)Pump.getPumpOnTimeLoops();
      uint64_t curEvents  = (uint64_t)Pump.getPumpEventCount();

      // Deltas since last 4HR boundary
      uint64_t deltaOnLoops =
        (curOnLoops >= last4HrPumpOnLoops) ? (curOnLoops - last4HrPumpOnLoops) : 0ULL;

      uint64_t deltaCycles =
        (curEvents >= last4HrPumpEventCount) ? (curEvents - last4HrPumpEventCount) : 0ULL;

      // Convert ON time to gallons
      float deltaOnMin = ((float)deltaOnLoops / (float)LOOPS_PER_SEC) / 60.0f;
      uint32_t deltaGallons = (uint32_t)(deltaOnMin * (float)GAL_PER_MIN + 0.5f);

      uint32_t totalGallons = (uint32_t)deltaGallons;
      uint32_t totalCycles  = (uint32_t)deltaCycles;
      float lastCycleEnergy = Pump.getLastCycleEnergyAmpSeconds();
      char dayKey[16];
      getCurrentDayKey(dayKey, sizeof(dayKey));

      uint8_t idx = getStoredDailyBlockCount() + 1 % NUM_4HR_BLOCKS;

      // Store 4HR block
      GAL_4HR[idx]   = totalGallons;
      CYCLE_4HR[idx] = totalCycles;
      GAL_TODAY   += totalGallons;
      CYCLE_TODAY += totalCycles;

      // Advance baseline snapshot for next block
      last4HrPumpOnLoops    = curOnLoops;
      last4HrPumpEventCount = curEvents;

      // Next test-mode write cadence
      sim4HrEvent.setMin(5);

      persist4HrStateToNvm(idx);
      Serial.printf("4HR BLOCK SAVED TO NVM: blocks=%u day=%04u_%02u_%02u epoch=%lu gal=%lu cyc=%lu\n",
        getStoredDailyBlockCount(),
        getYearInt(), getMonthInt(), getDayInt(),
        (uint32_t)time(nullptr),
        GAL_4HR[idx],
        CYCLE_4HR[idx]);

      nvmDumpPumpState();

      // Daily rollover after last block of the day

        // Add to 365 Chart Data
        Pump.Daily365.recordDailySummary((int)CYCLE_TODAY, (int)GAL_TODAY);
        gYesterdayWasZero = (CYCLE_TODAY == 0);

        clearDailyPumpData();
        nvmSetZeroBlocks();
      }
    }

  // -----------------------
  // NORMAL MODE
  // -----------------------
  else {
    const float hoursSinceBoot = ((float)LOOP_COUNT / (float)LOOPS_PER_SEC) / 3600.0f; 
    bool writeBlock = false;

    // Snapshot current cumulative counters ONLY at the block boundary
    uint64_t curOnLoops = (uint64_t)Pump.getPumpOnTimeLoops();
    uint64_t curEvents  = (uint64_t)Pump.getPumpEventCount();

    // Block totals since last 4HR boundary
    uint64_t blockOnLoops =
      (curOnLoops >= last4HrPumpOnLoops) ? (curOnLoops - last4HrPumpOnLoops) : 0ULL;

    uint64_t blockCycles =
      (curEvents >= last4HrPumpEventCount) ? (curEvents - last4HrPumpEventCount) : 0ULL;

    // Convert ON time to gallons
    float blockOnMin = ((float)blockOnLoops / (float)LOOPS_PER_SEC) / 60.0f;
    uint32_t blockGallons = (uint32_t)(blockOnMin * (float)GAL_PER_MIN + 0.5f);

    uint8_t idx = 100;
    if      (hour == 3)  idx = 0;
    else if (hour == 7)  idx = 1;
    else if (hour == 11) idx = 2;
    else if (hour == 15) idx = 3;
    else if (hour == 19) idx = 4;
    else if (hour == 23) idx = 5;

    bool validBlock = (idx >= 0 && idx < NUM_4HR_BLOCKS);

    if (hour != lastHour && validBlock) {
      // Compute Block Data

      // 1) try observed-rate estimate if enough current-block coverage
      if (hoursSinceBoot >= 3.5f) {
        writeBlock = true;
      }
      // 2) else use prior completed block if it exists
      else if (getStoredDailyBlockCount() > 0 && 
        getStoredDailyBlockCount() <= NUM_4HR_BLOCKS && idx > 0) { 
        uint8_t prev = idx - 1 ;
        blockGallons = GAL_4HR[prev];
        blockCycles  = CYCLE_4HR[prev];
        writeBlock = true;
      }
      // 3) else leave empty, do not increment count
      else {
        writeBlock = false;
        setDataType(DATA_PARTIAL);
      }

      // Save the data if appropriate
      if (writeBlock) {
        // Store 4HR block
        GAL_4HR[idx]   = blockGallons;
        CYCLE_4HR[idx] = blockCycles;
        GAL_TODAY     += blockGallons;
        CYCLE_TODAY   += blockCycles;

        persist4HrStateToNvm(idx);
        Serial.printf("4HR BLOCK WRITTEN: idx=%u day=%04u_%02u_%02u epoch=%lu gal=%lu cyc=%lu\n",
          (uint8_t)idx,
          getYearInt(), getMonthInt(), getDayInt(),
          (uint32_t)time(nullptr),
          GAL_4HR[idx],
          CYCLE_4HR[idx]);

        if (hour == 23) {
          uint32_t sumGal = GAL_TODAY;
          uint32_t sumCyc = CYCLE_TODAY;
          float lastCycleEnergy = Pump.getLastCycleEnergyAmpSeconds();
          char dayKey[16];
          getCurrentDayKey(dayKey, sizeof(dayKey));

          // Write Data to 365 Day Buffer        
          Pump.Daily365.recordDailySummary((int)sumCyc, (int)sumGal);

          gYesterdayWasZero = (CYCLE_TODAY == 0);
          clearDailyPumpData();
          nvmSetZeroBlocks();
        }
      }  // end of write block

      // Advance baseline snapshot for next block
      last4HrPumpOnLoops    = curOnLoops;
      last4HrPumpEventCount = curEvents;

    } // end of new 4 hour check

    lastHour = hour;
  }
}

