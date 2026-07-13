// nvm.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#define DAY_KEY_SIZE 16

// ---- Lifecycle ----
void nvmInit();   // optional; safe to call once in setup()

// ----  block state ----
// Saves: has_state, day_key, save_epoch
bool nvmSaveState(
  const char* dayKey,           // "YYYY_MM_DD"
  uint32_t saveEpoch            // epoch seconds (0 ok, but restore-window won't work)
);


//  Block Methods
void nvmClearState();
void nvmDumpLoggerState();
void nvmDumpBootState();

// Getters
uint32_t getTotalBlockWriteCount();


// ---- Boot stats (update once per reboot) ----
// Increments boot_count and updates last_boot_epoch / prev_boot_epoch.
void nvmUpdateBootStats(uint32_t nowEpoch);

// Read boot stats (optional convenience)
uint32_t nvmGetBootCount();
uint32_t nvmGetLastBootEpoch();
uint32_t nvmGetPrevBootEpoch();