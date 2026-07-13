#include "global.h"
#include "logger.h"

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

  minuteIndex = 0;

  inEvent = false;
  eventsDetected = false;

  currentDuration = 0;
}

void EventLogger::clearHourBlock()
{
  memset(&hourStats, 0, sizeof(hourStats));

  if (inEvent)
    hourStats.flags |= EVENT_CONTINUES;
}

const EventLogger::EventStatistics&
EventLogger::getHourStatistics() const
{
  return hourStats;
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
void EventLogger::sample(bool active)
{
  header.samplesTaken++;

  if (active)
  {
      inEvent = true;
      currentDuration++;
  }
  else if (inEvent)
  {
      finishEvent();
  }
}

void EventLogger::endHourBlock()
{
  //
  // Finish this block if an event is still active.
  // Continue timing the event into the next block.
  //
  if (inEvent)
  {
    finishEvent();

    inEvent = true;
    currentDuration = 0;

    hourStats.flags |= EVENT_CONTINUES;
  }

  // Save completed minute into current hour.
  if (minuteIndex < 60)
  {
    currentHour.minute[minuteIndex] = hourStats;
    minuteIndex++;
  }

  clearHourBlock();

  // Hour complete.
  if (minuteIndex >= 60)
  {
      minuteIndex = 0;
      endHour();
  }
}

bool EventLogger::endHour()
{
  // Logger only prepares the completed hour.
  // Flash storage is handled elsewhere.

  header.hoursStored++;

  return true;
}

void EventLogger::finishEvent()
{
  if (currentDuration == 0)
    return;

  eventsDetected = true;

  //------------------------------------------------
  // HourBlock statistics
  //------------------------------------------------

  hourStats.count++;

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

  sessionStats.count++;

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


