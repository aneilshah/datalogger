#pragma once

enum LoggerSimulation
{
  LOGGER_SIM_FLAT,
  LOGGER_SIM_RAMP,
  LOGGER_SIM_BURST,
};

void loggerSimulateAddingData();
void loggerReadWriteDataTest();
void loggerDumpNVMHourBlock(uint16_t hourNumber,  bool detailed = false);
void loggerDumpRAMHourBlock(bool detailed = false);
void loggerDumpNVMHeader();
void loggerDumpRAMHeader();
void loggerCreateAndWriteTestNVMData();
bool loggerSimulateHour(LoggerSimulation type, uint32_t eventsPerMinute, 
  uint32_t duration, uint8_t minutes = 60);
static bool simulateMinute(uint32_t eventCount, uint32_t duration);