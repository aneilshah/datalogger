//*****************************************************************************
// Test Code
//*****************************************************************************
#include "global.h"
#include <WiFi.h>

// Project Files
#include "logger.h"
#include "loggerData.h"
#include "mode.h"
#include "ntp.h"
#include "wifiFunc.h"

//*****************************************************************************
// Logger Test
//*****************************************************************************
void loggerTest()
{
  loggerDataErase();
  Logger.clear();

  // Build one hour of test data
  for (int minute = 0; minute < 60; minute++)
  {
    Logger.clearBucket();

    switch (minute % 6)
    {
      case 0:
        for (int i = 0; i < 5; i++)
          Logger.sample(true);

        Logger.sample(false);
        break;

      case 1:
        for (int i = 0; i < 10; i++)
          Logger.sample(true);

        Logger.sample(false);
        break;

      case 2:
        for (int i = 0; i < 5; i++)
          Logger.sample(true);

        Logger.sample(false);

        for (int i = 0; i < 10; i++)
          Logger.sample(false);

        for (int i = 0; i < 5; i++)
          Logger.sample(true);

        Logger.sample(false);
        break;

      case 3:
        for (int i = 0; i < 30; i++)
          Logger.sample(true);

        Logger.sample(false);
        break;

      case 4:
        break;

      case 5:
        for (int j = 0; j < 3; j++)
        {
          for (int i = 0; i < 10; i++)
            Logger.sample(true);

          Logger.sample(false);

          for (int i = 0; i < 5; i++)
            Logger.sample(false);
        }
        break;
    }

    Logger.endBucket();
  }

  // Save first hour
  Logger.endHour();

  // Read header
  EventLogger::LogHeader header;

  if (loggerDataReadHeader(header))
  {
    Serial.printf("Hours Stored : %u\n", header.hoursStored);
    Serial.printf("Samples      : %lu\n", header.samplesTaken);
  }

  // Read first hour
  EventLogger::HourRecord hourRecord;

  if (!loggerDataReadHour(0, hourRecord))
  {
    Serial.println("Read failed");
    return;
  }

  // Dump first hour
  Serial.println();
  Serial.println("Min  Cnt  Tot  Min  Max  Flg");
  Serial.println("----------------------------");

  for (int i = 0; i < 60; i++)
  {
    const auto &m = hourRecord.minute[i];

    Serial.printf("%02d  ", i);
    Serial.printf("Cnt=%2lu  ", m.count);
    Serial.printf("Tot=%3lu  ", m.total);
    Serial.printf("Min=%2lu  ", m.shortest);
    Serial.printf("Max=%2lu  ", m.longest);
    Serial.printf("Flg=%u\n", m.flags);
  }

  Serial.println();
  Serial.println("Logger test complete.");
}

//*****************************************************************************
// Logger Data Test
//*****************************************************************************
void loggerDataTest()
{
  EventLogger::LogHeader txHeader;
  EventLogger::LogHeader rxHeader;

  memset(&txHeader, 0, sizeof(txHeader));
  memset(&rxHeader, 0, sizeof(rxHeader));

  txHeader.magic = 0x12345678;
  txHeader.version = 1;

  strcpy(txHeader.startTime, "2026-07-13 17:30");
  strcpy(txHeader.stopTime,  "2026-07-13 18:30");

  txHeader.samplesTaken = 3600;
  txHeader.hoursStored = 1;
  txHeader.crc = 0x55AA;

  Serial.println();
  Serial.println("===== LOGGER DATA TEST =====");

  loggerDataErase();

  if (!loggerDataWriteHeader(txHeader))
  {
    Serial.println("Header write failed");
    return;
  }

  if (!loggerDataReadHeader(rxHeader))
  {
    Serial.println("Header read failed");
    return;
  }

  Serial.printf("Magic        : %08lX\n", rxHeader.magic);
  Serial.printf("Version      : %u\n", rxHeader.version);
  Serial.printf("Start Time   : %s\n", rxHeader.startTime);
  Serial.printf("Stop Time    : %s\n", rxHeader.stopTime);
  Serial.printf("Samples      : %lu\n", rxHeader.samplesTaken);
  Serial.printf("Hours Stored : %u\n", rxHeader.hoursStored);
  Serial.printf("CRC          : %04X\n", rxHeader.crc);

  Serial.println("===== TEST COMPLETE =====");
}

//*****************************************************************************
// Logger Data Test - 3 Hours
//*****************************************************************************
void loggerDataTest3Hours()
{
  EventLogger::LogHeader header;
  EventLogger::HourRecord hour;

  memset(&header, 0, sizeof(header));

  header.magic = 0x12345678;
  header.version = 1;
  strcpy(header.startTime, "2026-07-13 08:00");
  strcpy(header.stopTime,  "2026-07-13 11:00");
  header.samplesTaken = 3 * 3600;
  header.hoursStored = 3;
  header.crc = 0x55AA;

  Serial.println();
  Serial.println("===== LOGGER DATA TEST =====");

  loggerDataErase();

  if (!loggerDataWriteHeader(header))
  {
    Serial.println("Header write failed");
    return;
  }

  //------------------------------------------------
  // Hour 0
  //------------------------------------------------

  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    hour.minute[i].count = 1;
    hour.minute[i].shortest = 10;
    hour.minute[i].longest = 10;
    hour.minute[i].total = 10;
  }

  loggerDataWriteHour(0, hour);

  //------------------------------------------------
  // Hour 1
  //------------------------------------------------

  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    hour.minute[i].count = 2;
    hour.minute[i].shortest = 20;
    hour.minute[i].longest = 20;
    hour.minute[i].total = 40;
  }

  loggerDataWriteHour(1, hour);

  //------------------------------------------------
  // Hour 2
  //------------------------------------------------

  memset(&hour, 0, sizeof(hour));

  for (int i = 0; i < 60; i++)
  {
    hour.minute[i].count = 3;
    hour.minute[i].shortest = 30;
    hour.minute[i].longest = 30;
    hour.minute[i].total = 90;
  }

  loggerDataWriteHour(2, hour);

  Serial.println("3 Hours Written");
}

//*****************************************************************************
// Dump Hour
//*****************************************************************************
void loggerDumpHour(uint16_t hourNumber)
{
  EventLogger::HourRecord hour;

  if (!loggerDataReadHour(hourNumber, hour))
  {
    Serial.printf("Read Hour %u Failed\n", hourNumber);
    return;
  }

  Serial.println();
  Serial.printf("===== HOUR %u =====\n", hourNumber);
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