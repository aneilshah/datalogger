#pragma once

#include <stdint.h>

// Lifecycle
void eventDataInit();
void eventDataClear();

// Add events
bool eventDataAddNow();
void eventDataAddMinute(uint32_t minute2026);
void eventDataPurgeNow();


// Info
int eventDataCount();
bool eventDataEmpty();

// Access (0 = oldest, count-1 = newest)
bool eventDataGetMinute(int index, uint32_t& outMinute2026);

// Helpers
bool eventDataValidClock();
uint32_t eventDataGetMinutesSince2026();