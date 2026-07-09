// OTA.h
#pragma once

#define OTA_OFF 0
#define OTA_INIT 1
#define OTA_ON 2
#define OTA_FLASHING 3

// OTA Handler
void initOTA(const char* hostname);
void handleOTA();
bool otaInProgress();

// OTA State
unsigned int getOtaState();
void reinitOta();
void updateOTAState();
