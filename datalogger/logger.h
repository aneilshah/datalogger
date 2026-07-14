#pragma once

#include <stdint.h>


enum HourFlags : uint8_t
{
  HOUR_NORMAL   = 0x00,
  HOUR_PARTIAL  = 0x01,
  HOUR_PAUSED   = 0x02,
  HOUR_REBOOT   = 0x04,
};

enum MinuteFlags : uint8_t
{
   FLAG_NONE      = 0x00,
   FLAG_PARTIAL   = 0x01,
   FLAG_PAUSED    = 0x02,
   FLAG_NO_TIME   = 0x04,   // optional
};

class EventLogger;

//-----------------------------------------------------
// Event Logger
//-----------------------------------------------------

class EventLogger
{
public:

  struct EventStatistics
  {
    uint32_t count = 0;
    uint32_t shortest = 0;
    uint32_t longest = 0;
    uint32_t total = 0;
    uint8_t  flags = 0;

    float average() const
    {
      return count ? (float)total / count : 0.0f;
    }
      bool empty() const
    {
      return count == 0;
    }

    bool hasEvents() const
    {
      return count != 0;
    }
  };

  struct HourRecord
  {
    EventStatistics minute[60];
    uint8_t flags = 0;
    uint8_t reserved[3] = {0};   // future-proof / alignment
  };

  struct LogHeader
  {
    uint32_t magic = 0;
    uint16_t version = 1;

    char startTime[17] = {0};
    char stopTime[17] = {0};

    uint32_t samplesTaken = 0;
    uint16_t hoursStored = 0;
    uint8_t flags = 0;

    uint16_t crc = 0;
  };

  EventLogger();

  // Add one sample (typically once per second)
  void sample(bool active);

  // Finish current minute
  bool endMinuteBlock();

  // Finish current hour
  bool endHour();

  // Clear current minute
  void clearMinuteStats();

  // Clear hour stats
  void clearHour();

  // Reset entire logging session
  void clear();

  const EventStatistics& getMinuteStatistics() const;
  const EventStatistics& getHourStatistics() const;
  const EventStatistics& getSessionStatistics() const;

  const HourRecord& getHourRecord() const;
  const LogHeader& getHeader() const;

  bool hasEvents() const;

private:

  void startEvent();
  void finishEvent();


  EventStatistics hourStats;
  EventStatistics minuteStats;
  EventStatistics sessionStats;

  HourRecord currentHour;
  LogHeader header;

  bool inEvent = false;
  bool eventsDetected = false;

  uint8_t minuteIndex = 0;
  uint32_t currentDuration = 0;
};