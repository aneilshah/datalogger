#pragma once

#include "logger.h"

bool loggerFlashInit();
bool loggerFlashErase();

bool loggerFlashWriteHeader(
  const EventLogger::LogHeader &header);

bool loggerFlashReadHeader(
  EventLogger::LogHeader &header);

bool loggerFlashAppendHour(
  const EventLogger::HourRecord &hour);

bool loggerReadHour(
  uint16_t hourIndex,
  EventLogger::HourRecord &hour);