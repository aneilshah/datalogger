#pragma once

#define DAY_KEY_SIZE 16

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
extern char dayKey[DAY_KEY_SIZE];

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
