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

bool loggerDataWriteHourBlock(
  uint16_t hour,
  const EventLogger::HourRecord &hourRecord);

bool loggerDataReadHourBlock(
  uint16_t hour,
  EventLogger::HourRecord &hourRecord);