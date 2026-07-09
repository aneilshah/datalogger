#pragma once

#define BLOCK_DAY_KEY_SIZE 16
#define DATA_SYSTEM 0    // All Data from System
#define DATA_ESTIMATED 1 // Data uses estimates
#define DATA_PARTIAL 2   // Block Skipped

// external pump variables
extern int PUMP_EVENT;
extern float PUMP_CPH;
extern int PUMP_GPD;
extern int cycleLockoutCount;
extern uint32_t GAL_TODAY;
extern uint32_t CYCLE_TODAY;
extern uint32_t GAL_4HR[];
extern uint32_t CYCLE_4HR[];
extern bool gYesterdayWasZero;
extern char blockDayKey[BLOCK_DAY_KEY_SIZE];

// getters
uint8_t getDataType();
void setDataType(uint8_t data_type);

// Pump Defines
#define LOCKOUT_TIME_LOOPS 10 * LOOPS_PER_SEC  // 10 sec [No second pump event during this time]
#define TEST_NEXT_EVT_MIN_SEC 50   
#define TEST_NEXT_EVT_MAX_SEC 70   

// Pump Functions
void processPumpEvent();
void checkForPumpEvent();
void initPump();
void processFBWriteEvent();
