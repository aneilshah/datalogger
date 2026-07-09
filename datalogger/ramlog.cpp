#include "ramlog.h"
#include "global.h"

extern char* getTimestamp();

char gLogMsg[LOG_LINES][LOG_LEN] = {{0}};
char gLogTs[LOG_LINES][TS_LEN] = {{0}};
uint8_t gLogIndex = 0;
uint8_t gLogCount = 0;

void ramLog(const char* msg) {
  const char* ts = getTimestamp();
  if (!ts) ts = "?";

  snprintf(gLogTs[gLogIndex], sizeof(gLogTs[gLogIndex]), "%s", ts);
  snprintf(gLogMsg[gLogIndex], sizeof(gLogMsg[gLogIndex]), "%s", msg ? msg : "");

  gLogIndex = (gLogIndex + 1) % LOG_LINES;
  if (gLogCount < LOG_LINES) gLogCount++;
}

void ramLogf(const char* fmt, ...) {
  char msg[64];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  const char* ts = getTimestamp();
  if (!ts) ts = "?";

  snprintf(gLogTs[gLogIndex], sizeof(gLogTs[gLogIndex]), "%s", ts);
  snprintf(gLogMsg[gLogIndex], sizeof(gLogMsg[gLogIndex]), "%s", msg);

  gLogIndex = (gLogIndex + 1) % LOG_LINES;
  if (gLogCount < LOG_LINES) gLogCount++;
}

