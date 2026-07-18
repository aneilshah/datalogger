#pragma once

#include <stdint.h>

enum MinuteFlags : uint8_t
{
  MINUTE_FLAG_NONE     = 0x00,
  MINUTE_FLAG_PARTIAL  = 0x01,
  MINUTE_FLAG_REBOOT   = 0x02,
  MINUTE_FLAG_PAUSED   = 0x08
};

enum HourFlags : uint8_t
{
  HOUR_FLAG_NONE       = 0x00,
  HOUR_FLAG_PARTIAL    = 0x01,
  HOUR_FLAG_REBOOT     = 0x02,
};

enum SessionFlags : uint8_t
{
  SESSION_FLAG_NONE       = 0x00,
  SESSION_FLAG_REBOOT     = 0x02,
  SESSION_FLAG_RECOVERED  = 0x08
};

//*****************************************************************************
// Logger Flag Text Lookup
//*****************************************************************************
struct LoggerFlagText
{
  uint8_t flag;
  const char *text;
};

//*****************************************************************************
// Minute Flag Text
//*****************************************************************************
static const LoggerFlagText minuteFlagTable[] =
{
  { MINUTE_FLAG_PARTIAL, "PARTIAL" },
  { MINUTE_FLAG_PAUSED,  "PAUSED"  },
  { MINUTE_FLAG_REBOOT,  "REBOOT"  },
};

//*****************************************************************************
// Hour Flag Text
//*****************************************************************************
static const LoggerFlagText hourFlagTable[] =
{
  { HOUR_FLAG_PARTIAL, "PARTIAL" },
  { HOUR_FLAG_REBOOT,  "REBOOT"  },
};

static const LoggerFlagText sessionFlagTable[] =
{
  { SESSION_FLAG_REBOOT, "REBOOTED" },
  { SESSION_FLAG_RECOVERED,"RECOVERED"},
};

// Flag Helpers
const char *getHourFlagText(uint8_t flags);
const char *getMinuteFlagText(uint8_t flags);
const char *getSessionFlagText(uint8_t flags);

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
    char startTime[LOGGER_TIMESTAMP_LENGTH];
    char stopTime[LOGGER_TIMESTAMP_LENGTH];
    uint8_t flags = 0;
    uint8_t reserved[3] = {0};   // future-proof / alignment
  };

  struct LogHeader
  {
    uint32_t magic = 0;
    uint16_t version = 1;

    char startTime[LOGGER_TIMESTAMP_LENGTH] = {0};
    char stopTime[LOGGER_TIMESTAMP_LENGTH] = {0};

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

  // Start / Stop current hour
  bool startHour();
  bool endHour();

  // Clear current minute
  void clearMinuteStats();

  // Clear hour stats
  void clearHour();

  // Reset entire logging session
  void clearRam();

  // Session Functions
  void startNewSession();
  void restartSession();
  void stopSession();

  // Statuses
  bool isLoggingActive();
  bool isLoggingPaused();
  bool hasEvents() const;

  // Getters
  const EventStatistics& getMinuteStatistics() const;
  const EventStatistics& getHourStatistics() const;
  const EventStatistics& getSessionStatistics() const;
  const HourRecord& getHourRecord() const;
  const LogHeader& getRamHeader() const;

  // Setters
  bool setRamHeader(const LogHeader& header);
  
private:

  void startEvent();
  void finishEvent();

  EventStatistics hourStats;
  EventStatistics minuteStats;
  EventStatistics sessionStats;

  HourRecord currentHour;
  LogHeader ramHeader;

  bool inEvent = false;
  bool eventsDetected = false;

  uint8_t minuteIndex = 0;
  uint32_t currentDuration = 0;
};