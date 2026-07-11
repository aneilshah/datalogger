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
char dayKey[16] = {0};


// Methods
void processPumpEvent() {
  // Check for new pump event
  if (PUMP_EVENT && !cycleLockoutCount) {
    Pump.processPumpOnEvent(LOOP_COUNT);
    Serial.println("[PUMP] ON");

    cycleLockoutCount = LOCKOUT_TIME_LOOPS;
    ledOn();
  }

  // Clear Event
  PUMP_EVENT = 0;

  // Check for lockout timeout
  if (cycleLockoutCount == 1) {
    ledOff();
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
  dayKey[0] = '\0';
}
