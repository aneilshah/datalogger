#include "global.h"
#include "logger.h"

#include <string.h>

EventLogger::EventLogger()
{
  clear();
}

void EventLogger::clear()
{
  memset(&bucketStats, 0, sizeof(bucketStats));
  memset(&sessionStats, 0, sizeof(sessionStats));
  memset(&currentHour, 0, sizeof(currentHour));
  memset(&header, 0, sizeof(header));

  minuteIndex = 0;

  inEvent = false;
  eventsDetected = false;

  currentDuration = 0;
}

void EventLogger::clearBucket()
{
  memset(&bucketStats, 0, sizeof(bucketStats));

  if (inEvent)
    bucketStats.flags |= EVENT_CONTINUES;
}

const EventLogger::EventStatistics&
EventLogger::getBucketStatistics() const
{
  return bucketStats;
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

void EventLogger::endBucket()
{
  //
  // Finish this bucket if an event is still active.
  // Continue timing the event into the next bucket.
  //
  if (inEvent)
  {
    finishEvent();

    inEvent = true;
    currentDuration = 0;

    bucketStats.flags |= EVENT_CONTINUES;
  }

  // Save completed minute into current hour.
  if (minuteIndex < 60)
  {
    currentHour.minute[minuteIndex] = bucketStats;
    minuteIndex++;
  }

  clearBucket();

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
  // Bucket statistics
  //------------------------------------------------

  bucketStats.count++;

  if (bucketStats.shortest == 0 ||
    currentDuration < bucketStats.shortest)
  {
    bucketStats.shortest = currentDuration;
  }

  if (currentDuration > bucketStats.longest)
  {
    bucketStats.longest = currentDuration;
  }

  bucketStats.total += currentDuration;

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


