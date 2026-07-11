// nvm.h
#pragma once
#include <stdint.h>
#include <stddef.h>

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

// Getters
uint32_t getTotalBlockWriteCount();


// ---- Boot stats (update once per reboot) ----
// Increments boot_count and updates last_boot_epoch / prev_boot_epoch.
void nvmUpdateBootStats(uint32_t nowEpoch);

// Read boot stats (optional convenience)
uint32_t nvmGetBootCount();
uint32_t nvmGetLastBootEpoch();
uint32_t nvmGetPrevBootEpoch();