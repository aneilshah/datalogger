#pragma once

enum LoggerSimulation
{
  LOGGER_SIM_FLAT,
  LOGGER_SIM_RAMP,
  LOGGER_SIM_BURST,
};

void loggerSimulateAddingData();
void loggerReadWriteDataTest();
void loggerDumpHourBlock(uint16_t hourNumber,  bool detailed = false);
void loggerDumpNVMHeader();
void loggerDumpRAMHeader();
void loggerCreateAndWriteTestNVMData();
bool loggerSimulateHour(LoggerSimulation type, uint32_t eventsPerMinute, uint32_t duration);
static bool simulateMinute(uint32_t eventCount, uint32_t duration);