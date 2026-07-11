#pragma once

#include <stdint.h>

//-----------------------------------------------------
// Logger Modes
//-----------------------------------------------------

#define LG_INIT     0
#define LG_LOGGING  1
#define LG_PAUSED   2

enum : uint8_t
{
    EVENT_CONTINUES = 0x01
};

//-----------------------------------------------------
// Globals
//-----------------------------------------------------

uint8_t getLoggerMode();

void initLogger();
void processLogger();

class EventLogger;
extern EventLogger Logger;

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
    };

    struct HourRecord
    {
        EventStatistics minute[60];
    };

    struct LogHeader
    {
        uint32_t magic = 0;
        uint16_t version = 1;

        char startTime[17] = {0};
        char stopTime[17] = {0};

        uint32_t samplesTaken = 0;
        uint16_t hoursStored = 0;

        uint16_t crc = 0;
    };

    EventLogger();

    // Add one sample (typically once per second)
    void sample(bool active);

    // Finish current minute
    void endBucket();

    // Finish current hour
    bool endHour();

    // Clear current minute
    void clearBucket();

    // Reset entire logging session
    void clear();

    const EventStatistics& getBucketStatistics() const;
    const EventStatistics& getSessionStatistics() const;

    const HourRecord& getHourRecord() const;
    const LogHeader& getHeader() const;

    bool hasEvents() const;

private:

    void finishEvent();

    EventStatistics bucketStats;
    EventStatistics sessionStats;

    HourRecord currentHour;
    LogHeader header;

    bool inEvent = false;
    bool eventsDetected = false;

    uint8_t minuteIndex = 0;
    uint32_t currentDuration = 0;
};