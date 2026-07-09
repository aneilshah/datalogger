// nvm.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#define INVALID_BLOCK 0  // WANT THIS TO BE 255

typedef enum : uint8_t {
  NVM_RESTORE_OK = 0,
  NVM_RESTORE_BAD_ARGS,
  NVM_RESTORE_PREFS_FAIL,
  NVM_RESTORE_NO_STATE,
  NVM_RESTORE_DAY_MISMATCH,
  NVM_RESTORE_SAVED_EPOCH_ZERO,
  NVM_RESTORE_NOW_EPOCH_ZERO,
  NVM_RESTORE_TOO_OLD,
  NVM_RESTORE_BYTES_MISMATCH
} NvmRestoreResult;

// ---- Lifecycle ----
void nvmInit();   // optional; safe to call once in setup()

// ---- 4-hour block state ----
// Saves: has_state, day_key, save_epoch, block_count first_block, GAL_4HR[6], CYCLE_4HR[6]
bool nvmSave4hrState(
  const char* dayKey,           // "YYYY_MM_DD"
  uint32_t saveEpoch,           // epoch seconds (0 ok, but restore-window won't work)
  uint8_t idx,
  const uint32_t* gal4hr, size_t gal4hrCount,
  const uint32_t* cyc4hr, size_t cyc4hrCount
);

// Restores if:
// - saved state exists
// - dayKey matches
// - and (nowEpoch - saveEpoch) <= maxAgeSeconds  (if both epochs non-zero)
NvmRestoreResult nvmRestore4hrStateIfFresh(
  const char* dayKey,
  uint32_t nowEpoch,
  uint32_t maxAgeSeconds,
  uint32_t* outGal4hr, size_t gal4hrCount,
  uint32_t* outCyc4hr, size_t cyc4hrCount
);


//  Block Methods
void nvmClear4hrState();
void nvmDumpPumpState();
bool nvmSetZeroBlocks();

// Getters
uint32_t getTotalBlockWriteCount();
uint8_t getLastStoredBlockHour();
uint8_t getFirst4hrBlockIdx();
uint8_t getStoredDailyBlockCount();


void logNVMFailure(NvmRestoreResult result);

// ---- Boot stats (update once per reboot) ----
// Increments boot_count and updates last_boot_epoch / prev_boot_epoch.
void nvmUpdateBootStats(uint32_t nowEpoch);

// Read boot stats (optional convenience)
uint32_t nvmGetBootCount();
uint32_t nvmGetLastBootEpoch();
uint32_t nvmGetPrevBootEpoch();