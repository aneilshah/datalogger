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

bool loggerDataWriteNvmHeader(
  const EventLogger::LogHeader &ramHeader);

bool loggerDataReadNvmHeader(
  EventLogger::LogHeader &nvmHeader);

bool loggerDataWriteHourBlock(
  uint16_t hour,
  const EventLogger::HourRecord &hourRecord);

bool loggerDataReadHourBlock(
  uint16_t hour,
  EventLogger::HourRecord &hourRecord);