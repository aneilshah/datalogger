#include "global.h"
#include "logger.h"
#include "loggerData.h"
#include "mode.h"
#include "ntp.h"
#include "nvm.h"

// Just in case
#include <string.h>


// Flag Helpers

//*****************************************************************************
// Logger Flags to Text
//*****************************************************************************
const char *loggerFlagsToText(
  uint8_t flags,
  const LoggerFlagText *table,
  size_t count)
{
  static char text[64];

  text[0] = '\0';

  for (size_t i = 0; i < count; i++)
  {
    if (flags & table[i].flag)
    {
      if (text[0] != '\0')
        strcat(text, "|");

      strcat(text, table[i].text);
    }
  }

  if (text[0] == '\0')
    strcpy(text, "NONE");

  return text;
}

//*****************************************************************************
// Get Minute Flag Text
//*****************************************************************************
const char *getMinuteFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    minuteFlagTable,
    sizeof(minuteFlagTable) / sizeof(minuteFlagTable[0]));
}

//*****************************************************************************
// Get Hour Flag Text
//*****************************************************************************
const char *getHourFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    hourFlagTable,
    sizeof(hourFlagTable) / sizeof(hourFlagTable[0]));
}

//*****************************************************************************
// Get Session Flag Text
//*****************************************************************************
const char *getSessionFlagText(uint8_t flags)
{
  return loggerFlagsToText(
    flags,
    sessionFlagTable,
    sizeof(sessionFlagTable) / sizeof(sessionFlagTable[0]));
}

// Setters

void EventLogger::setSessionFlag(uint16_t flag)
{
  ramHeader.flags |= flag;
}

void EventLogger::clearSessionFlag(uint16_t flag)
{
  ramHeader.flags &= ~flag;
}



EventLogger::EventLogger()
{
  clearRam();
}

void EventLogger::clearRam()
{
  memset(&hourStats, 0, sizeof(hourStats));
  memset(&sessionStats, 0, sizeof(sessionStats));
  memset(&currentHour, 0, sizeof(currentHour));
  memset(&ramHeader, 0, sizeof(ramHeader));
  ramHeader.magic   = LOGGER_MAGIC;
  ramHeader.version = LOGGER_VERSION;

  minuteIndex = 0;

  inEvent = false;
  eventsDetected = false;

  currentDuration = 0;
}

void EventLogger::clearHour() 
{
  memset(&currentHour, 0, sizeof(currentHour));
  memset(&hourStats, 0, sizeof(hourStats));
  minuteIndex = 0;

  if (ramHeader.hoursStored >= LOGGER_MAX_HOURS)
  {
    stopSession();
    return;
  }

  // Initialize all minutes as "not yet occurred"
  for (int i = 0; i < 60; i++)
    currentHour.minute[i].flags = MINUTE_FLAG_NO_DATA;

  // Start the next Hour
  startHour();
}

void EventLogger::clearMinuteStats()
{
  memset(&minuteStats, 0, sizeof(minuteStats));
}

const EventLogger::EventStatistics&
EventLogger::getHourStatistics() const
{
  return hourStats;
}

const EventLogger::EventStatistics&
EventLogger::getMinuteStatistics() const
{
  return minuteStats;
}

const EventLogger::EventStatistics&
EventLogger::getSessionStatistics() const
{
  return sessionStats;
}

const EventLogger::HourRecord& EventLogger::getHourRecord() const
{
  return currentHour;
}

const EventLogger::LogHeader& EventLogger::getRamHeader() const
{
  return ramHeader;
}

bool EventLogger::setRamHeader(const LogHeader& header)
{
    ramHeader = header;
    return true;
}

bool EventLogger::hasEvents() const
{
  return eventsDetected;
}

void EventLogger::startNewSession()
{
  // Set the Ram header
  strncpy(ramHeader.startTime, getTimestamp(), sizeof(ramHeader.startTime) - 1);
  ramHeader.startTime[sizeof(ramHeader.startTime) - 1] = '\0';
  ramHeader.stopTime[0] = '\0';

  // Save session header into NVM
  loggerDataWriteNvmHeader(ramHeader);

  // Set the Boot State
  NvmBootState boot = {};
  boot.loggerMode = LoggerMode::LOGGING;
  boot.hoursStored   = 0;
  strncpy(boot.saveTimestamp, getTimestamp(), sizeof(boot.saveTimestamp) - 1);
  boot.saveTimestamp[sizeof(boot.saveTimestamp) - 1] = '\0';
  bootStateWrite(boot);

  // Initialize
  startHour();

  Serial.println("Starting Session...");
}

void restartSession() {

}

void EventLogger::stopSession()
{
  const char *timestamp = getTimestamp();

  // Set the RAM header
  strncpy(
    ramHeader.stopTime,
    timestamp,
    sizeof(ramHeader.stopTime) - 1);

  ramHeader.stopTime[sizeof(ramHeader.stopTime) - 1] = '\0';

  // Update Boot State
  NvmBootState boot;

  if (bootStateRead(boot))
  {
    boot.loggerMode = LoggerMode::STOPPED;
    strncpy(
      boot.saveTimestamp,
      timestamp,
      sizeof(boot.saveTimestamp) - 1);

    boot.saveTimestamp[sizeof(boot.saveTimestamp) - 1] = '\0';

    bootStateWrite(boot);
  }

  Serial.println("Stopping Session...");
}

void EventLogger::sample(bool active)
{
    ramHeader.samplesTaken++;

    // Check Leading edge
    if (active && !inEvent)
    {
      startEvent();
    }

    // Count active second
    if (inEvent)
    {
      currentDuration++;
    }

    // Check Trailing edge
    if (!active && inEvent)
    {
      finishEvent();
    }
}

bool EventLogger::endMinuteBlock()
{
  if (minuteIndex >= 60)
  {
    Serial.println("ERROR: minuteIndex out of range");
    return false;
  }

  if (getLoggerMode() == LoggerMode::PAUSED) {
    minuteStats.flags |= MINUTE_FLAG_PAUSED;
    currentHour.flags |= HOUR_FLAG_PAUSED;
    ramHeader.flags |= SESSION_FLAG_PAUSED;
  }

  // Minute is now complete. (Clear NO_DATA)
  minuteStats.flags &= ~MINUTE_FLAG_NO_DATA;

  currentHour.minute[minuteIndex] = minuteStats;
  minuteIndex++;

  clearMinuteStats();

  return true;
}

bool EventLogger::startHour()
{
  strncpy(
    currentHour.startTime,
    getTimestamp(),
    sizeof(currentHour.startTime) - 1);

  currentHour.stopTime[0] = '\0';
  return true;
}

bool EventLogger::endHour()
{
  // Logger only prepares the completed hour.
  // Flash storage is handled elsewhere.
  strncpy(
    currentHour.stopTime,
    getTimestamp(),
    sizeof(currentHour.stopTime) - 1);
  ramHeader.currentHourIdx++;

  // Set partial flag if hour has less than 60 minutes
  if (minuteIndex < 60)
  {
    currentHour.flags |= HOUR_FLAG_PARTIAL;
    ramHeader.flags |= SESSION_FLAG_PARTIAL;
  }

  return true;
}

void EventLogger::startEvent() {
  eventsDetected = true;

  minuteStats.count++;
  hourStats.count++;
  sessionStats.count++;

  inEvent = true;
  currentDuration = 0;
}

void EventLogger::finishEvent()
{
  if (currentDuration == 0)
    return;

  //------------------------------------------------
  // Minute statistics
  //------------------------------------------------

  if (minuteStats.shortest == 0 ||
      currentDuration < minuteStats.shortest)
  {
    minuteStats.shortest = currentDuration;
  }

  if (currentDuration > minuteStats.longest)
  {
    minuteStats.longest = currentDuration;
  }

  minuteStats.total += currentDuration;

  //------------------------------------------------
  // Hour statistics
  //------------------------------------------------

  if (hourStats.shortest == 0 ||
      currentDuration < hourStats.shortest)
  {
    hourStats.shortest = currentDuration;
  }

  if (currentDuration > hourStats.longest)
  {
    hourStats.longest = currentDuration;
  }

  hourStats.total += currentDuration;

  //------------------------------------------------
  // Session statistics
  //------------------------------------------------

  if (sessionStats.shortest == 0 ||
      currentDuration < sessionStats.shortest)
  {
    sessionStats.shortest = currentDuration;
  }

  if (currentDuration > sessionStats.longest)
  {
    sessionStats.longest = currentDuration;
  }

  sessionStats.total += currentDuration;

  currentDuration = 0;
  inEvent = false;
}

void EventLogger::incrementHoursStored() {
  ramHeader.hoursStored++;
}
