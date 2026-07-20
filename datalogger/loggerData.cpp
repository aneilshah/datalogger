//*****************************************************************************
// loggerData.cpp
//*****************************************************************************
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "logger.h"
#include "loggerData.h"
#include "mode.h"
#include "ntp.h"
#include "nvm.h"

static Preferences prefs;

//*****************************************************************************
// Constants
//*****************************************************************************
static const char *NS_LOGGER = "logger";
static const char *K_HEADER  = "header";

//*****************************************************************************
// Build Hour Key
//*****************************************************************************
static void makeHourKey(uint16_t hour, char *key)
{
  snprintf(key, 16, "hour%03u", hour);
}

//*****************************************************************************
// Initialize
//*****************************************************************************
bool loggerDataInit()
{
  return true;
}

//*****************************************************************************
// Erase Logger Data
//*****************************************************************************
bool loggerDataErase()
{
  Serial.println("Erasing NVM Data");
  if (!prefs.begin(NS_LOGGER, false)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  prefs.clear();
  prefs.end();

  return true;
}

//*****************************************************************************
// Write NVM Header
//*****************************************************************************
bool loggerDataWriteNvmHeader(const EventLogger::LogHeader &ramHeader)
{
  Serial.println("Writing Data Header");
  if (!prefs.begin(NS_LOGGER, false)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  bool ok =
    (prefs.putBytes(
      K_HEADER,
      &ramHeader,
      sizeof(ramHeader)) == sizeof(ramHeader));

  prefs.end();

  if (ok)
    Serial.println("Success Writing NVM Data Header");
  else
    Serial.println("ERROR - NVM Header Write Failed");

  return ok;
}

//*****************************************************************************
// Read NVM Header
//*****************************************************************************
bool loggerDataReadNvmHeader(EventLogger::LogHeader &nvmHeader)
{
  Serial.println("Reading NVM Data Header");
  if (!prefs.begin(NS_LOGGER, true)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  bool ok =
    (prefs.getBytes(
      K_HEADER,
      &nvmHeader,
      sizeof(nvmHeader)) == sizeof(nvmHeader));

  prefs.end();

  return ok;
}

//*****************************************************************************
// Write Hour
//*****************************************************************************
bool loggerDataWriteHourBlock(
  uint16_t hour,
  const EventLogger::HourRecord &hourRecord)
{
  char key[16];

  Serial.printf("Writing Data Hour %u\n", hour);
  makeHourKey(hour, key);

  if (!prefs.begin(NS_LOGGER, false)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  bool ok =
    (prefs.putBytes(
      key,
      &hourRecord,
      sizeof(hourRecord)) == sizeof(hourRecord));

  prefs.end();

  return ok;
}

//*****************************************************************************
// Read Hour Block
//*****************************************************************************
bool loggerDataReadHourBlock(
  uint16_t hour,
  EventLogger::HourRecord &hourRecord)
{
  char key[16];

  Serial.printf("Reading Data Hour %u\n", hour);
  makeHourKey(hour, key);

  if (!prefs.begin(NS_LOGGER, true)) {
    Serial.printf("ERROR Accessing NS_LOGGER NVM");
    return false;
  }

  bool ok =
    (prefs.getBytes(
      key,
      &hourRecord,
      sizeof(hourRecord)) == sizeof(hourRecord));

  prefs.end();

  return ok;
}

// Complete the Hours
bool loggerDataFinishHour()
{
  // Finish the current hour
  if (!Logger.endHour())
    return false;

  // Save completed hour to NVM
  if (!loggerDataWriteHourBlock(
    Logger.getRamHeader().currentHourIdx - 1, // already incremented
    Logger.getHourRecord()))
  {
    // For now keep hourStored in sync with timestamp, regardless of success
    Logger.incrementHoursStored();  
    return false;
  }
  else
    Logger.incrementHoursStored();

  // Save session header into NVM
  loggerDataWriteNvmHeader(Logger.getRamHeader());

  // Update boot state
  NvmBootState boot;

  if (bootStateRead(boot))
  {
    boot.hoursStored = Logger.getRamHeader().hoursStored;

    strncpy(
      boot.saveTimestamp,
      getTimestamp(),
      sizeof(boot.saveTimestamp) - 1);

    boot.saveTimestamp[sizeof(boot.saveTimestamp) - 1] = '\0';

    bootStateWrite(boot);
  }

  // Prepare next hour
  Logger.clearHour();
  Logger.startHour();

  return true;
}

bool checkpointNvm()
{
  bool ok = true;
  
  // Save Current Hour
  Logger.incrementHoursStored();
  ok &= loggerDataWriteHourBlock(
    Logger.getRamHeader().currentHourIdx, 
    Logger.getHourRecord()
  );
  
  //  save session NVM
  ok &= loggerDataWriteNvmHeader(Logger.getRamHeader());

  Serial.printf("Checkpoint NVM - hour: %d status:%u\n", Logger.getRamHeader().currentHourIdx, ok);
  return ok; 
}