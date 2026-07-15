//*****************************************************************************
// Test Code
//*****************************************************************************
#include "global.h"
#include "testcode.h"
#include <WiFi.h>

// Project Files
#include "logger.h"
#include "loggerData.h"
#include "mode.h"
#include "ntp.h"
#include "wifiFunc.h"

// HELPERS

//*****************************************************************************
// Build Flat Hour Block
//*****************************************************************************
void loggerBuildFlatHourBlock(
  EventLogger::HourRecord &hour,
  uint32_t count,
  uint32_t duration)
{
  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    hour.minute[i].count = count;
    hour.minute[i].shortest = duration;
    hour.minute[i].longest = duration;
    hour.minute[i].total = count * duration;
  }
}

//*****************************************************************************
// Build Ramp Hour
//*****************************************************************************
void loggerBuildRampHourBlock(
  EventLogger::HourRecord &hour,
  uint32_t maxCount,
  uint32_t duration)
{
  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    uint32_t count = ((i + 1) * maxCount) / 60;

    if (count == 0)
      count = 1;

    hour.minute[i].count = count;
    hour.minute[i].shortest = duration;
    hour.minute[i].longest = duration;
    hour.minute[i].total = count * duration;
  }
}

//*****************************************************************************
// Build Burst Hour
//*****************************************************************************
void loggerBuildBurstHourBlock(
  EventLogger::HourRecord &hour,
  uint32_t burstCount,
  uint32_t duration)
{
  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    uint32_t count = 0;

    if (i >= 20 && i < 40)
      count = burstCount;

    hour.minute[i].count = count;

    if (count > 0)
    {
      hour.minute[i].shortest = duration;
      hour.minute[i].longest  = duration;
      hour.minute[i].total    = count * duration;
    }
  }
}


//*****************************************************************************
// Create Test NVM Data
//*****************************************************************************
void loggerCreateAndWriteTestNVMData()
{
  loggerDataErase();

  EventLogger::LogHeader ramHeader;
  EventLogger::HourRecord hour;

  memset(&ramHeader, 0, sizeof(ramHeader));

  ramHeader.magic = LOGGER_MAGIC;
  ramHeader.version = LOGGER_VERSION;

  strcpy(ramHeader.startTime, "2026-07-13 08:00");
  strcpy(ramHeader.stopTime,  "2026-07-13 11:00");

  ramHeader.samplesTaken = 3 * 3600;
  ramHeader.hoursStored = 3;
  ramHeader.crc = 0;

  if (!loggerDataWriteNvmHeader(ramHeader))
  {
    Serial.println("NVM Header write failed");
    return;
  }

  //------------------------------------------------
  // Hour 0 - Flat
  //------------------------------------------------

  loggerBuildFlatHourBlock(hour, 1, 10);

  if (!loggerDataWriteHourBlock(0, hour))
  {
    Serial.println("Hour 0 write failed");
    return;
  }

  //------------------------------------------------
  // Hour 1 - Ramp
  //------------------------------------------------

  loggerBuildRampHourBlock(hour, 12, 10);

  if (!loggerDataWriteHourBlock(1, hour))
  {
    Serial.println("Hour 1 write failed");
    return;
  }

  //------------------------------------------------
  // Hour 2 - Burst
  //------------------------------------------------

  loggerBuildBurstHourBlock(hour, 8, 15);

  if (!loggerDataWriteHourBlock(2, hour))
  {
    Serial.println("Hour 2 write failed");
    return;
  }

  Serial.println("Logger test data created.");
}

//////////////////////////////////////////////////////
// Simulate Data Run Helpers
//////////////////////////////////////////////////////

static void loggerDumpTestResults(uint16_t hours)
{
  loggerDumpNVMHeader();

  for (uint16_t hour = 0; hour < hours; hour++)
    loggerDumpNVMHourBlock(hour, true);
}

static bool saveCurrentHour()
{
  if (!Logger.endHour())
    return false;

  if (!loggerDataWriteHourBlock(
      Logger.getRamHeader().hoursStored - 1,
      Logger.getHourRecord()))
  {
    return false;
  }

  if (!loggerDataWriteNvmHeader(Logger.getRamHeader()))
  {
    return false;
  }

  Logger.clearHour();

  return true;
}

//*****************************************************************************
// Logger Simulate Adding Data
//*****************************************************************************
void loggerSimulateAddingData()
{
  if (!resetLogger())
  {
    Serial.println("Reset Logger Failed");
    return;
  }

  // START SESSION
  Logger.startSession();

  // Simulate Hour 1
  if (!loggerSimulateHour(LOGGER_SIM_FLAT, 1, 10))
  {
    Serial.println("Flat Hour Failed");
    return;
  }

  if (!saveCurrentHour())
  {
    Serial.println("Save Hour 0 Failed");
    return;
  }

  // Simulate Hour 2
  if (!loggerSimulateHour(LOGGER_SIM_RAMP, 10, 10))
  {
    Serial.println("Ramp Hour Failed");
    return;
  }

  if (!saveCurrentHour())
  {
    Serial.println("Save Hour 1 Failed");
    return;
  }

  // Simulate Hour 3
  if (!loggerSimulateHour(LOGGER_SIM_BURST, 5, 10))
  {
    Serial.println("Burst Hour Failed");
    return;
  }

  if (!saveCurrentHour())
  {
    Serial.println("Save Hour 2 Failed");
    return;
  }

  // And add a partial hour of recording
  if (!loggerSimulateHour(LOGGER_SIM_BURST, 5, 11, 25))
  {
    Serial.println("Burst Half Hour Failed");
    return;
  }

  // STOP SESSION
  delay(5000);
  Logger.stopSession();
  loggerDataWriteNvmHeader(Logger.getRamHeader()); // NVM

  loggerDumpNVMHeader();
  loggerDumpNVMHourBlock(0, false);  // true = minute details, false = no details
  loggerDumpNVMHourBlock(1, false);
  loggerDumpNVMHourBlock(2, false);
  loggerDumpRAMHourBlock(false);

  Serial.println();
  Serial.println("Logger test complete.");
}


static bool simulateMinute(uint32_t eventCount, uint32_t duration)
{
  for (uint32_t event = 0; event < eventCount; event++)
  {
    for (uint32_t second = 0; second < duration; second++)
      Logger.sample(true);

    Logger.sample(false);
  }

  return Logger.endMinuteBlock();
}

bool loggerSimulateHour(LoggerSimulation type, uint32_t eventsPerMinute, 
  uint32_t duration, uint8_t minutes) 
{
  for (uint8_t minute = 0; minute < minutes; minute++)
  {
    uint32_t minuteCount = 0;

    switch (type)
    {
      case LOGGER_SIM_FLAT:
        minuteCount = eventsPerMinute;
        break;

      case LOGGER_SIM_RAMP:
        minuteCount = ((minute + 1) * eventsPerMinute) / 60;

        if (minuteCount == 0)
          minuteCount = 1;
        break;

      case LOGGER_SIM_BURST:
        if (minute >= 20 && minute < 40)
          minuteCount = eventsPerMinute;
        break;
      
      default:
        return false;
    }

    if (!simulateMinute(minuteCount, duration))
      return false;
  }
  return true;
}


//*****************************************************************************
// Logger Read Write Data Test
//*****************************************************************************
void loggerReadWriteDataTest()
{
  EventLogger::LogHeader txRamHeader;
  EventLogger::LogHeader rxNvmHeader;

  memset(&txRamHeader, 0, sizeof(txRamHeader));
  memset(&rxNvmHeader, 0, sizeof(rxNvmHeader));

  txRamHeader.magic = LOGGER_MAGIC;
  txRamHeader.version = 1;

  strcpy(txRamHeader.startTime, "2026-07-13 17:30");
  strcpy(txRamHeader.stopTime,  "2026-07-13 18:30");

  txRamHeader.samplesTaken = 3600;
  txRamHeader.hoursStored = 1;
  txRamHeader.crc = 0x55AA;

  Serial.println();
  Serial.println("===== LOGGER DATA TEST =====");

  loggerDataErase();

  if (!loggerDataWriteNvmHeader(txRamHeader))
  {
    Serial.println("Header write failed");
    return;
  }

  if (!loggerDataReadNvmHeader(rxNvmHeader))
  {
    Serial.println("NVM Header read failed");
    return;
  }

  Serial.printf("Magic        : %08lX\n", rxNvmHeader.magic);
  Serial.printf("Version      : %u\n", rxNvmHeader.version);
  Serial.printf("Start Time   : %s\n", rxNvmHeader.startTime);
  Serial.printf("Stop Time    : %s\n", rxNvmHeader.stopTime);
  Serial.printf("Samples      : %lu\n", rxNvmHeader.samplesTaken);
  Serial.printf("Hours Stored : %u\n", rxNvmHeader.hoursStored);
  Serial.printf("CRC          : %04X\n", rxNvmHeader.crc);

  Serial.println("===== TEST COMPLETE =====");
}


//*****************************************************************************
// Dump Hour
//*****************************************************************************
void loggerDumpNVMHourBlock(uint16_t hourNumber, bool detailed)
{
  EventLogger::HourRecord hour;

  if (!loggerDataReadHourBlock(hourNumber, hour))
  {
    Serial.printf("Read Hour %u Failed\n", hourNumber);
    return;
  }

  Serial.println();
  Serial.printf("===== HOUR %u =====\n", hourNumber);

  if (!detailed)
  {
    uint32_t totalEvents = 0;
    uint32_t totalSeconds = 0;
    uint32_t longest = 0;
    uint32_t shortest = UINT32_MAX;

    for (int i = 0; i < 60; i++)
    {
      const auto &m = hour.minute[i];

      totalEvents += m.count;
      totalSeconds += m.total;

      if (m.count > 0)
      {

        if (m.longest > longest)
          longest = m.longest;

        if (m.shortest < shortest)
          shortest = m.shortest;
      }
    }

    Serial.printf("Start    : %s\n", hour.startTime);
    Serial.printf("Stop     : %s\n", hour.stopTime);
    Serial.printf("Events   : %lu\n", totalEvents);
    Serial.printf("Duration : %lu sec\n", totalSeconds);

    if (totalEvents > 0)
    {
      Serial.printf("Shortest : %lu sec\n", shortest);
      Serial.printf("Longest  : %lu sec\n", longest);
      Serial.printf("Average  : %.1f sec\n",
        (float)totalSeconds / totalEvents);
    }
    else
    {
      Serial.println("Shortest : ***");
      Serial.println("Longest  : ***");
      Serial.println("Average  : ***");
    }

    return;
  }

  Serial.println("Min  Cnt  Tot  Min  Max  Flg");
  Serial.println("----------------------------");

  for (int i = 0; i < 60; i++)
  {
    const auto &m = hour.minute[i];

    Serial.printf("%02d   ", i);
    Serial.printf("%2lu   ", m.count);
    Serial.printf("%3lu   ", m.total);
    Serial.printf("%3lu   ", m.shortest);
    Serial.printf("%3lu   ", m.longest);
    Serial.printf("%u\n", m.flags);
  }
}

//*****************************************************************************
// Dump NVM Header
//*****************************************************************************
void loggerDumpNVMHeader()
{
  EventLogger::LogHeader nvmHeader;

  if (!loggerDataReadNvmHeader(nvmHeader))
  {
    Serial.println("Read NVM Header Failed");
    return;
  }

  Serial.println();
  Serial.println("===== HEADER =====");

  Serial.printf("Magic        : %08lX\n", nvmHeader.magic);
  Serial.printf("Version      : %u\n", nvmHeader.version);
  Serial.printf("Hours Stored : %u\n", nvmHeader.hoursStored);
  Serial.printf("Samples      : %lu\n", nvmHeader.samplesTaken);
  Serial.printf("Start Time   : %s\n", nvmHeader.startTime);
  Serial.printf("Stop Time    : %s\n", nvmHeader.stopTime);
  Serial.printf("Flags        : %u\n", nvmHeader.flags);
}

//*****************************************************************************
// Dump RAM Header
//*****************************************************************************
void loggerDumpRAMHeader()
{
  const EventLogger::LogHeader &ramHeader = Logger.getRamHeader();

  Serial.println();
  Serial.println("===== RAM HEADER =====");

  Serial.printf("Version      : %u\n", ramHeader.version);
  Serial.printf("Hours Stored : %u\n", ramHeader.hoursStored);
  Serial.printf("Samples      : %lu\n", ramHeader.samplesTaken);
  Serial.printf("Start Time   : %s\n", ramHeader.startTime);
  Serial.printf("Stop Time    : %s\n", ramHeader.stopTime);
  Serial.printf("Flags        : %u\n", ramHeader.flags);
}

//*****************************************************************************
// Dump RAM Hour Block
//*****************************************************************************
void loggerDumpRAMHourBlock(bool detailed)
{
  const EventLogger::HourRecord &hour = Logger.getHourRecord();

  Serial.println();
  Serial.println("===== RAM HOUR =====");

  if (!detailed)
  {
    uint32_t totalEvents = 0;
    uint32_t totalSeconds = 0;
    uint32_t longest = 0;

    for (int i = 0; i < 60; i++)
    {
      const auto &m = hour.minute[i];

      totalEvents += m.count;
      totalSeconds += m.total;

      if (m.longest > longest)
        longest = m.longest;
    }

    Serial.printf("Events   : %lu\n", totalEvents);
    Serial.printf("Duration : %lu sec\n", totalSeconds);
    Serial.printf("Longest  : %lu sec\n", longest);

    return;
  }

  Serial.println("Min  Cnt  Tot  Min  Max  Flg");
  Serial.println("----------------------------");

  for (int i = 0; i < 60; i++)
  {
    const auto &m = hour.minute[i];

    Serial.printf("%02d   ", i);
    Serial.printf("%2lu   ", m.count);
    Serial.printf("%3lu   ", m.total);
    Serial.printf("%3lu   ", m.shortest);
    Serial.printf("%3lu   ", m.longest);
    Serial.printf("%u\n", m.flags);
  }
}