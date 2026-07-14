// nvm.cpp
#include <Preferences.h>
#include <string.h>
#include "global.h"
#include "ntp.h"
#include "nvm.h"

static Preferences prefs;

// Namespaces
static const char* NS_LOGGER = "logger";
static const char* NS_BOOT = "boot";

// Keys (logger namespace)
static const char* K_HAS_STATE  = "has_state";
static const char* K_DAY_KEY    = "day_key";
static const char* K_SAVE_EPOCH = "save_epoch";
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


bool nvmSaveState(
  const char* dayKey,
  uint32_t saveEpoch
) {
  if (!dayKey) return false;

  if (!prefs.begin(NS_LOGGER, false)) return false;
  
  // Get updated write counts
  uint32_t oldWriteCount = prefs.getULong(K_WRITE_COUNT, 0);
  uint32_t newWriteCount = oldWriteCount + 1;  // incremenet explicitly
  uint8_t hour = getHourInt();

  bool ok = true;
  ok &= prefs.putBool(K_HAS_STATE, true);
  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, (unsigned long)saveEpoch) > 0);
  ok &= (prefs.putULong(K_WRITE_COUNT, newWriteCount) > 0);

  prefs.end();
  return ok;
}

void nvmClearState() {  
  if (!prefs.begin(NS_LOGGER, false)) return;

  // Dont remove K_WRITE_COUNT
  prefs.remove(K_HAS_STATE);
  prefs.remove(K_DAY_KEY);
  prefs.remove(K_SAVE_EPOCH);
  prefs.end();
}

bool nvmSetZeroBlocks() {
  bool ok = true;

  if (!prefs.begin(NS_LOGGER, false)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  // Default / clean values
  char dayKey[DAY_KEY_SIZE]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));

  ok &= prefs.putBool(K_HAS_STATE, true);

  ok &= (prefs.putString(K_DAY_KEY, dayKey) > 0);
  ok &= (prefs.putULong(K_SAVE_EPOCH, nowEpoch) > 0);

  // Dont change K_WRITE_COUNT

  prefs.end();
  return ok;
}

//-------------------------------------------------
// NVM Getters
//-------------------------------------------------

uint32_t getTotalBlockWriteCount() {
  if (!prefs.begin(NS_LOGGER, true)) {return 0;}
  const uint32_t v = (uint32_t)prefs.getULong(K_WRITE_COUNT, 0);
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

void nvmDumpLoggerState()
{
  Serial.println("----- NVM LOGGER DUMP BEGIN -----");

  if (!prefs.begin(NS_LOGGER, true)) {
    Serial.println("Error Accessing NS_LOGGER NVM");
    return;
  }

  bool hasState = prefs.getBool(K_HAS_STATE, false);
  String dayKey = prefs.getString(K_DAY_KEY, "");
  uint32_t saveEpoch = (uint32_t)prefs.getULong(K_SAVE_EPOCH, 0);

  Serial.printf("has_state: %s\n", hasState ? "true" : "false");
  Serial.printf("day_key:   %s\n", dayKey.c_str());
  Serial.printf("epoch:     %lu\n", (unsigned long)saveEpoch);
  prefs.end();

  Serial.println("----- NVM LOGGER DUMP END -----");
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

