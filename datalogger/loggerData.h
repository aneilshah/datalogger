//*****************************************************************************
// loggerData.h
//*****************************************************************************
#pragma once

#include <stdint.h>
#include "logger.h"

//*****************************************************************************
// Interface
//*****************************************************************************

bool loggerDataInit();
bool loggerDataErase();

bool loggerDataWriteHeader(
  const EventLogger::LogHeader &header);

bool loggerDataReadHeader(
  EventLogger::LogHeader &header);

bool loggerDataWriteHour(
  uint16_t hour,
  const EventLogger::HourRecord &hourRecord);

bool loggerDataReadHour(
  uint16_t hour,
  EventLogger::HourRecord &hourRecord);