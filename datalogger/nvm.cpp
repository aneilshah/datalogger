// nvm.cpp
#include <Preferences.h>
#include <string.h>
#include "global.h"
#include "logger.h"
#include "ntp.h"
#include "nvm.h"

// For Debug Only
#include <nvs.h>
#include <nvs_flash.h>

static Preferences prefs;

// Namespaces
static const char* NS_LOGGER = "logger";
static const char* NS_BOOT = "boot";

// Keys (logger namespace)
static const char* K_WRITE_COUNT = "write_count";  // Export this one

// Keys (boot namespace)
static const char* K_BOOT_COUNT     = "boot_count";
static const char* K_BOOT_STATE     = "boot_state";


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
  ok &= (prefs.putULong(K_WRITE_COUNT, newWriteCount) > 0);

  prefs.end();
  return ok;
}

void nvmClearState() {  
  if (!prefs.begin(NS_LOGGER, false)) return;

  // Dont remove K_WRITE_COUNT
  prefs.end();
}

bool nvmSetZeroBlocks() {
  bool ok = true;

  if (!prefs.begin(NS_LOGGER, false)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  // Dont change K_WRITE_COUNT

  prefs.end();
  return ok;
}

// HELPERS
const char* getBootLoggerModeTxt() {
  LoggerMode mode = getBootLoggerMode();
  if (mode == LoggerMode::RESET) return "INIT";
  if (mode == LoggerMode::LOGGING) return "LOGGING";
  if (mode == LoggerMode::STOPPED) return "STOPPED";
  else return "UNKNOWN_MODE";
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

//*****************************************************************************
// Read Boot State
//*****************************************************************************
bool bootStateRead(NvmBootState &boot)
{
  if (!prefs.begin(NS_BOOT, true))
    return false;

  size_t len = prefs.getBytesLength(K_BOOT_STATE);

  if (len != sizeof(NvmBootState))
  {
    prefs.end();
    return false;
  }

  bool ok = (prefs.getBytes(
    K_BOOT_STATE,
    &boot,
    sizeof(NvmBootState)) == sizeof(NvmBootState));

  prefs.end();
  return ok;
}

//*****************************************************************************
// Write Boot State
//*****************************************************************************
bool bootStateWrite(const NvmBootState &boot)
{
  if (!prefs.begin(NS_BOOT, false))
    return false;

  bool ok = (prefs.putBytes(
    K_BOOT_STATE,
    &boot,
    sizeof(NvmBootState)) == sizeof(NvmBootState));

  prefs.end();
  return ok;
}

//*****************************************************************************
// Clear Boot State
//*****************************************************************************
bool bootStateClear()
{
  if (!prefs.begin(NS_BOOT, false))
    return false;

  bool ok = prefs.remove(K_BOOT_STATE);

  prefs.end();
  return ok;
}

bool bootStateReset()
{
  NvmBootState boot = {};

  boot.loggerMode = LoggerMode::RESET;
  boot.hoursStored   = 0;
  boot.saveTimestamp[0] = '\0';

  return bootStateWrite(boot);
}

void nvmUpdateBootStats(uint32_t nowEpoch) {
  if (!prefs.begin(NS_BOOT, false)) return;

  const uint32_t bootCount = (uint32_t)prefs.getUInt(K_BOOT_COUNT, 0);
  prefs.putUInt(K_BOOT_COUNT, bootCount + 1);
  prefs.end();
}

// Getters

uint32_t nvmGetBootCount() {
  if (!prefs.begin(NS_BOOT, true)) return 0;
  const uint32_t v = (uint32_t)prefs.getUInt(K_BOOT_COUNT, 0);
  prefs.end();
  return v;
}

LoggerMode getBootLoggerMode()
{
    NvmBootState boot{};

    if (!bootStateRead(boot))
        return LoggerMode::RESET;

    return boot.loggerMode;
}

// Setters

bool setBootLoggerMode(LoggerMode mode){
  // Update Boot State
  NvmBootState boot;

  if (bootStateRead(boot))
  {
    boot.loggerMode = mode;

    strncpy(
      boot.saveTimestamp,
      getTimestamp(),
      sizeof(boot.saveTimestamp) - 1);

    boot.saveTimestamp[sizeof(boot.saveTimestamp) - 1] = '\0';

    return bootStateWrite(boot);
  }
  return false;  
}

///////////////////////////////////////////////////
// Diagnostics
///////////////////////////////////////////////////

void nvmDumpLoggerState()
{
  Serial.println("----- NVM LOGGER DUMP BEGIN -----");

  if (!prefs.begin(NS_LOGGER, true)) {
    Serial.println("Error Accessing NS_LOGGER NVM");
    return;
  }
  uint32_t writeCount = prefs.getUInt(K_WRITE_COUNT, 0);
  Serial.printf("Write Count : %lu\n", (unsigned long)writeCount);
  prefs.end();

  Serial.println("----- NVM LOGGER DUMP END -----");
}

void nvmDumpBootState()
{
  Serial.println("----- NVM BOOT DUMP BEGIN -----");

  NvmBootState boot;

  if (!bootStateRead(boot))
  {
    Serial.println("Boot state not found.");
    Serial.println("----- NVM BOOT DUMP END -----");
    return;
  }

  Serial.printf("Logger Mode : %s\n", getBootLoggerModeTxt());
  Serial.printf("Hours Stored   : %u\n", boot.hoursStored);
  Serial.printf("Last Save      : %s\n", boot.saveTimestamp);
  Serial.println("----- NVM BOOT DUMP END -----");
}

void dumpNamespace(const char *ns)
{
    nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, ns, NVS_TYPE_ANY);
    while (it != nullptr)
    {
      nvs_entry_info_t info;
      nvs_entry_info(it, &info);

      Serial.printf(
        "Namespace: %s  Key: %s  Type: %d\n",
        info.namespace_name,
        info.key,
        info.type);

      it = nvs_entry_next(it);
    }

    nvs_release_iterator(it);
}