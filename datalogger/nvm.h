// nvm.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#define DAY_KEY_SIZE 16

// ---- Lifecycle ----
void nvmInit();   // optional; safe to call once in setup()

struct  NvmBootState
{
  bool    sessionActive;
  bool    sessionPaused;
  uint8_t sessionFlags;
  uint16_t hoursStored;

  char saveTimestamp[LOGGER_TIMESTAMP_LENGTH];
};


//  Block Methods
void nvmClearState();
void nvmDumpLoggerState();
void nvmDumpBootState();

bool bootStateRead(NvmBootState &boot);
bool bootStateWrite(const NvmBootState &boot);
bool bootStateClear();

// Getters
uint32_t getTotalBlockWriteCount();


// ---- Boot stats (update once per reboot) ----
// Increments boot_count
void nvmUpdateBootStats(uint32_t nowEpoch);

// Read boot stats (optional convenience)
uint32_t nvmGetBootCount();

bool bootStateReset();
