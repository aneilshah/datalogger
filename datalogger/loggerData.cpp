//*****************************************************************************
// loggerData.cpp
//*****************************************************************************
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "logger.h"
#include "loggerData.h"

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
  if (!prefs.begin(NS_LOGGER, false))
    return false;

  prefs.clear();
  prefs.end();

  return true;
}

//*****************************************************************************
// Write Header
//*****************************************************************************
bool loggerDataWriteHeader(const EventLogger::LogHeader &header)
{
  if (!prefs.begin(NS_LOGGER, false))
    return false;

  bool ok =
    (prefs.putBytes(
      K_HEADER,
      &header,
      sizeof(header)) == sizeof(header));

  prefs.end();

  return ok;
}

//*****************************************************************************
// Read Header
//*****************************************************************************
bool loggerDataReadHeader(EventLogger::LogHeader &header)
{
  if (!prefs.begin(NS_LOGGER, true))
    return false;

  bool ok =
    (prefs.getBytes(
      K_HEADER,
      &header,
      sizeof(header)) == sizeof(header));

  prefs.end();

  return ok;
}

//*****************************************************************************
// Write Hour
//*****************************************************************************
bool loggerDataWriteHour(
  uint16_t hour,
  const EventLogger::HourRecord &hourRecord)
{
  char key[16];

  makeHourKey(hour, key);

  if (!prefs.begin(NS_LOGGER, false))
    return false;

  bool ok =
    (prefs.putBytes(
      key,
      &hourRecord,
      sizeof(hourRecord)) == sizeof(hourRecord));

  prefs.end();

  return ok;
}

//*****************************************************************************
// Read Hour
//*****************************************************************************
bool loggerDataReadHour(
  uint16_t hour,
  EventLogger::HourRecord &hourRecord)
{
  char key[16];

  makeHourKey(hour, key);

  if (!prefs.begin(NS_LOGGER, true))
    return false;

  bool ok =
    (prefs.getBytes(
      key,
      &hourRecord,
      sizeof(hourRecord)) == sizeof(hourRecord));

  prefs.end();

  return ok;
}