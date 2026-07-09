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
static const char* K_BLK_CNT    = "blk_cnt";
static const char* K_FIRST_BLK  = "first_block";
static const char* K_LAST_HOUR  = "last_hour";
static const char* K_GAL4HR     = "gal4hr";
static const char* K_CYC4HR     = "cyc4hr";
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

bool nvmSave4hrState(
  const char* dayKey,
  uint32_t saveEpoch,
  uint8_t idx,
  const uint32_t* gal4hr, size_t gal4hrCount,
  const uint32_t* cyc4hr, size_t cyc4hrCount
) {
  if (!dayKey || !gal4hr || !cyc4hr) return false;
  if (gal4hrCount == 0 || cyc4hrCount == 0) return false;

  if (!prefs.begin(NS_PUMP, false)) return false;
  
  // Get updated write counts
  uint32_t oldWriteCount = prefs.getULong(K_WRITE_COUNT, 0);
  uint32_t newWriteCount = oldWriteCount + 1;  // incremenet explicitly
  uint8_t hour = getHourInt();
  uint8_t oldDailyCount = prefs.getUChar(K_BLK_CNT, INVALID_BLOCK);

  bool ok = true;
  ok &= prefs.putBool(K_HAS_STATE, true);
  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, (unsigned long)saveEpoch) > 0);
  ok &= (prefs.putUChar(K_LAST_HOUR, hour) > 0);
  ok &= (prefs.putULong(K_WRITE_COUNT, newWriteCount) > 0);

  // Update Block count on change
  if (oldDailyCount < NUM_4HR_BLOCKS && oldDailyCount != INVALID_BLOCK) {
    ok &= (prefs.putUChar(K_BLK_CNT, oldDailyCount + 1) > 0);
  } else {
    ok &= (prefs.putUChar(K_BLK_CNT, oldDailyCount) > 0);
  }

  // Save First Block on first write of day only
  if (oldDailyCount == 0) ok &= (prefs.putUChar(K_FIRST_BLK, idx) > 0);

  // Store arrays as raw bytes
  ok &= (prefs.putBytes(K_GAL4HR, gal4hr, gal4hrCount * sizeof(uint32_t)) == (gal4hrCount * sizeof(uint32_t)));
  ok &= (prefs.putBytes(K_CYC4HR, cyc4hr, cyc4hrCount * sizeof(uint32_t)) == (cyc4hrCount * sizeof(uint32_t)));

  prefs.end();

  if (ok) snprintf(blockDayKey, BLOCK_DAY_KEY_SIZE, "%s", dayKey);
  return ok;
}

NvmRestoreResult nvmRestore4hrStateIfFresh(
  const char* dayKey,
  uint32_t nowEpoch,
  uint32_t maxAgeSeconds,
  uint32_t* outGal4hr, size_t gal4hrCount,
  uint32_t* outCyc4hr, size_t cyc4hrCount) 
{
  if (!dayKey || !outGal4hr || !outCyc4hr) {
    return NVM_RESTORE_BAD_ARGS;
  }
  if (gal4hrCount == 0 || cyc4hrCount == 0) {
    return NVM_RESTORE_BAD_ARGS;
  }

  // Open NVS
  if (!prefs.begin(NS_PUMP, true)) return NVM_RESTORE_PREFS_FAIL;

  // Check State
  const bool has = prefs.getBool(K_HAS_STATE, false);
  if (!has) { prefs.end(); return NVM_RESTORE_NO_STATE; }

  // Check Day Key
  const String savedDay = prefs.getString(K_DAY_KEY, "");
  if (!safeStrEq(savedDay, dayKey)) { 
    prefs.end();
      Serial.printf("NVM day mismatch: saved='%s' requested='%s'\n",
        savedDay.c_str(), dayKey); 
    return NVM_RESTORE_DAY_MISMATCH;
  }

  // Check Epoch validity
  const uint32_t savedEpoch = (uint32_t)prefs.getULong(K_SAVE_EPOCH, 0);
  if (savedEpoch == 0) {
    Serial.println("NVM restore rejected: no saved epoch in NVM.");
    prefs.end();
    return NVM_RESTORE_SAVED_EPOCH_ZERO;
  }
  if (nowEpoch == 0) {
    Serial.println("NVM restore rejected: current time not valid.");
    prefs.end();
    return NVM_RESTORE_NOW_EPOCH_ZERO;
  }

  // Check NVM Age
  const uint32_t age_sec =
      (nowEpoch >= savedEpoch) ? (nowEpoch - savedEpoch) : 0xFFFFFFFFu;

  const int age_min = int(age_sec) / 60;

  if (age_sec > maxAgeSeconds) {
    Serial.printf("NVM restore rejected: age=%lu sec [%lu min] > max=%lu sec \n",
                  (unsigned long)age_sec,
                  (int) age_min,
                  (unsigned long)maxAgeSeconds);
    prefs.end();
    return NVM_RESTORE_TOO_OLD;
  } 

  // Restore arrays
  const size_t needGal = gal4hrCount * sizeof(uint32_t);
  const size_t needCyc = cyc4hrCount * sizeof(uint32_t);

  const size_t gotGal = prefs.getBytes(K_GAL4HR, outGal4hr, needGal);
  const size_t gotCyc = prefs.getBytes(K_CYC4HR, outCyc4hr, needCyc);

  prefs.end();

  const bool ok = (gotGal == needGal) && (gotCyc == needCyc);
  if (!ok) {
    Serial.printf("NVM restore failed: bytes mismatch gal=%u/%u cyc=%u/%u\n",
      (unsigned)gotGal, (unsigned)needGal,
      (unsigned)gotCyc, (unsigned)needCyc);
    return NVM_RESTORE_BYTES_MISMATCH;
  } 

  // Else Everything OK, Restore
  Serial.printf("NVM restore accepted: age=%ld min, blocks=%ud\n",
    (unsigned long)age_min, getStoredDailyBlockCount());
  return NVM_RESTORE_OK;
}

void nvmClear4hrState() {  
  if (!prefs.begin(NS_PUMP, false)) return;

  // Dont remove K_WRITE_COUNT
  prefs.remove(K_HAS_STATE);
  prefs.remove(K_DAY_KEY);
  prefs.remove(K_SAVE_EPOCH);
  prefs.remove(K_BLK_CNT);
  prefs.remove(K_FIRST_BLK);
  prefs.remove(K_LAST_HOUR);
  prefs.remove(K_GAL4HR);
  prefs.remove(K_CYC4HR);
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
  const uint8_t zeroBlkCnt = 0;
  const uint8_t zeroFirstBlk = 0;
  const uint8_t zeroLastHour = 0;

  // Create zero-filled temp buffers
  uint32_t zeroGal[NUM_4HR_BLOCKS] = {0};
  uint32_t zeroCyc[NUM_4HR_BLOCKS] = {0};

  ok &= prefs.putBool(K_HAS_STATE, true);

  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, nowEpoch) > 0);
  ok &= (prefs.putUChar(K_BLK_CNT, zeroBlkCnt) > 0);
  ok &= (prefs.putUChar(K_FIRST_BLK, zeroFirstBlk) > 0);
  ok &= (prefs.putUChar(K_LAST_HOUR, zeroLastHour) > 0);
  // Dont change K_WRITE_COUNT

  const size_t galBytes = sizeof(zeroGal);
  const size_t cycBytes = sizeof(zeroCyc);

  ok &= (prefs.putBytes(K_GAL4HR, zeroGal, galBytes) == galBytes);
  ok &= (prefs.putBytes(K_CYC4HR, zeroCyc, cycBytes) == cycBytes);

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

uint8_t getLastStoredBlockHour() {
  if (!prefs.begin(NS_PUMP, true)) {return INVALID_BLOCK;}
  const uint8_t v = (uint8_t)prefs.getUChar(K_LAST_HOUR, INVALID_BLOCK);
  prefs.end();
  return v;
}

uint8_t getStoredDailyBlockCount() {
  if (!prefs.begin(NS_PUMP, true)) {return INVALID_BLOCK;}
  const uint8_t v = (uint8_t)prefs.getUChar(K_BLK_CNT, 0);
  prefs.end();
  return v;
}

uint8_t getFirst4hrBlockIdx() {
  if (!prefs.begin(NS_PUMP, true)) {return INVALID_BLOCK;}
  const uint8_t v = (uint8_t)prefs.getUChar(K_FIRST_BLK, INVALID_BLOCK);
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

void logNVMFailure(NvmRestoreResult result)
{
  switch (result) {

    case NVM_RESTORE_NO_STATE:
      Serial.println("NVM RESTORE Failed: no saved nvm state");
      break;

    case NVM_RESTORE_DAY_MISMATCH:
      Serial.println("NVM RESTORE Failed: day mismatch");
      break;

    case NVM_RESTORE_TOO_OLD:
      Serial.println("NVM RESTORE Failed: Timestamp too Old");
      break;

    case NVM_RESTORE_NOW_EPOCH_ZERO:
      Serial.println("NVM RESTORE Failed: invalid system time");
      break;

    case NVM_RESTORE_SAVED_EPOCH_ZERO:
      Serial.println("NVM RESTORE Failed: invalid saved epoch");
      break;

    case NVM_RESTORE_BYTES_MISMATCH:
      Serial.println("NVM RESTORE Failed: array size mismatch");
      break;

    case NVM_RESTORE_PREFS_FAIL:
      Serial.println("NVM RESTORE Failed: prefs.begin failed");
      break;

    case NVM_RESTORE_BAD_ARGS:
      Serial.println("NVM RESTORE Failed: bad arguments.");
      break;

    default:
      Serial.println("NVM RESTORE Failed: unknown failure.");
      break;
  }
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
  uint8_t blkCnt = prefs.getInt(K_BLK_CNT, 0);
  uint8_t firstBlk = prefs.getInt(K_FIRST_BLK, 0);
  uint8_t lastHour = prefs.getInt(K_LAST_HOUR, 0);

  Serial.printf("has_state: %s\n", hasState ? "true" : "false");
  Serial.printf("day_key:   %s\n", dayKey.c_str());
  Serial.printf("epoch:     %lu\n", (unsigned long)saveEpoch);
  Serial.printf("blk_cnt:   %d\n", blkCnt);
  Serial.printf("first_blk: %d\n", firstBlk);
  Serial.printf("last_hour: %d\n", lastHour);

  // Dump arrays
  if (blkCnt > 0) {
    uint32_t gal[NUM_4HR_BLOCKS] = {0};   
    uint32_t cyc[NUM_4HR_BLOCKS] = {0};

    size_t needGal = sizeof(gal);
    size_t needCyc = sizeof(cyc);

    size_t gotGal = prefs.getBytes(K_GAL4HR, gal, needGal);
    size_t gotCyc = prefs.getBytes(K_CYC4HR, cyc, needCyc);

    Serial.printf("GAL4HR bytes: %u\n", (unsigned)gotGal);
    Serial.printf("CYC4HR bytes: %u\n", (unsigned)gotCyc);

    for (int i = 0; i < NUM_4HR_BLOCKS; i++) {
      Serial.printf("  Block %d: gal=%lu cyc=%lu\n",
                    i,
                    (unsigned long)gal[i],
                    (unsigned long)cyc[i]);
    }
  }

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

