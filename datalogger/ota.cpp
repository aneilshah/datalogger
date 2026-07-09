// ota.cpp
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "ota.h"
#include "global.h"
#include "wifiFunc.h"

static bool _otaInProgress = false;
static unsigned int otaState = OTA_OFF;

unsigned int getOtaState() {return otaState;}
void reinitOta() {otaState = OTA_INIT;}

void initOTA(const char* hostname)
{
  if (!MDNS.begin(hostname)) {
    Serial.println("mDNS failed");
  } else {
    Serial.println("mDNS started");
  }
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(3232);

  // Optional but strongly recommended
  //ArduinoOTA.setPassword("pumpOTA123");   // change this!

  ArduinoOTA.onStart([]() {
    _otaInProgress = true;
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
    _otaInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static int16_t lastPct = -5;

    int pct = (progress * 100) / total;

    if (pct - lastPct >= 5 || pct == 100) {
      Serial.printf("OTA Progress: %u%%\n", pct);
      lastPct = pct;
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      else Serial.println("Unknown");
      _otaInProgress = false;
    });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Host: %s\n", hostname);
  Serial.println("[OTA] Ready");
}

void updateOTAState() {
  bool wifiStable = wifiLinkReady();  

  switch (otaState) {
    case OTA_OFF:
      if (wifiStable) {
        Serial.println("[OTA] OFF -> ON");
        initOTA(TEST_MODE ? "pump-test" : "pump-prod");
        otaState = OTA_ON;
      }
      break;

    case OTA_ON:
      if (!wifiStable) {
        Serial.println("[OTA] ON -> INIT");
        otaState = OTA_INIT;
      }
      break;

    case OTA_INIT:
      if (wifiStable) {
        Serial.println("[OTA] INIT -> ON");
        initOTA(TEST_MODE ? "pump-test" : "pump-prod");
        otaState = OTA_ON;
      }
      break;
  }
}

void handleOTA()
{
  ArduinoOTA.handle();
}

bool otaInProgress()
{
  return _otaInProgress;
}