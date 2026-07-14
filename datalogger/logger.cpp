#include "global.h"
#include "logger.h"
#include "ntp.h"

#include <string.h>

EventLogger::EventLogger()
{
  clear();
}

void EventLogger::clear()
{
  memset(&hourStats, 0, sizeof(hourStats));
  memset(&sessionStats, 0, sizeof(sessionStats));
  memset(&currentHour, 0, sizeof(currentHour));
  memset(&header, 0, sizeof(header));
  header.magic   = LOGGER_MAGIC;
  header.version = LOGGER_VERSION;

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

const EventLogger::HourRecord&
EventLogger::getHourRecord() const
{
  return currentHour;
}

const EventLogger::LogHeader&
EventLogger::getHeader() const
{
  return header;
}

bool EventLogger::hasEvents() const
{
  return eventsDetected;
}

void EventLogger::startSession()
{
  strncpy(
    header.startTime,
    getTimestamp(),
    sizeof(header.startTime) - 1);

  header.startTime[sizeof(header.startTime) - 1] = '\0';

  header.stopTime[0] = '\0';
  Serial.println("Starting Session...");
}

void EventLogger::stopSession()
{
  strncpy(
    header.stopTime,
    getTimestamp(),
    sizeof(header.stopTime) - 1);

  header.stopTime[sizeof(header.stopTime) - 1] = '\0';
  Serial.println("Stopping Session");
}

void EventLogger::sample(bool active)
{
    header.samplesTaken++;

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

  currentHour.minute[minuteIndex] = minuteStats;
  minuteIndex++;

  clearMinuteStats();

  return true;
}

bool EventLogger::endHour()
{
  // Logger only prepares the completed hour.
  // Flash storage is handled elsewhere.
    header.hoursStored++;
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


