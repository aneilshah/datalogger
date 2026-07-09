#pragma once
#include <Arduino.h>
#include "ntp.h"

class Diag {
public:
  // Setters
  void setSystemState(const char* value) { setValue(systemState, value); }
  void setWifiState(const char* value) { setValue(wifiState, value); }
  void setWifiRSSI(const char* value) { setValue(wifiRSSI, value); }
  void setWifiIP(const char* value) { setValue(wifiIP, value); }
  void setWifiBSSID(const char* value) { setValue(wifiBSSID, value); }
  void setWifiDNS(const char* value) { setValue(wifiDNS, value); }
  void setWifiGW(const char* value) { setValue(wifiGW, value); }
  void setNTPState(const char* value) { setValue(ntpState, value); }
  void setPowerOnReason(const char* value) { setValue(powerOnReason, value); }
  void setHeapInfo(const char* value) { setValue(heapInfo, value); }
  void setNvmInfo(const char* value) { setValueLong(nvmInfo, value); }
  void setDiag1(const char* value) { setValue(diag1, value); }


  // Getters
  const char* getSystemState() const { return systemState; }
  const char* getWifiState() const { return wifiState; }
  const char* getWifiRSSI() const { return wifiRSSI; }
  const char* getWifiIP() const { return wifiIP; }
  const char* getWifiBSSID() const { return wifiBSSID; }
  const char* getWifiDNS() const { return wifiDNS; }
  const char* getWifiGW() const { return wifiGW; }
  const char* getNTPState() const { return ntpState; }

  // Updaters
  void updateDiagInfo() {
    // NTP
    if (!validClock()) {
      setNTPState("NTP_WAIT");
    } else {
      setNTPState("NTP_OK");
    }

    // Heap
    char buf[32];
    snprintf(buf, sizeof(buf), "%u / %u",
      ESP.getFreeHeap(),
      ESP.getMinFreeHeap());

    setHeapInfo(buf);
  } 

  Diag() {
    setDefault(systemState);
    setDefault(wifiState);
    setDefault(wifiRSSI);
    setDefault(wifiIP);
    setDefault(wifiBSSID);
    setDefault(wifiGW);
    setDefault(wifiDNS);
    setDefault(ntpState);
    setDefault(powerOnReason);
    setDefault(heapInfo);
    setDefault(nvmInfo);
    setDiag1("[unused]");
}

private:
  static const uint16_t DIAG_BUF_SIZE = 32;
  static const uint16_t DIAG_BUF_LONG = 160;

  char systemState[DIAG_BUF_SIZE];
  char wifiState[DIAG_BUF_SIZE];
  char wifiRSSI[DIAG_BUF_SIZE];
  char wifiIP[DIAG_BUF_SIZE];
  char wifiBSSID[DIAG_BUF_SIZE];
  char wifiGW[DIAG_BUF_SIZE];
  char wifiDNS[DIAG_BUF_SIZE];
  char ntpState[DIAG_BUF_SIZE];
  char powerOnReason[DIAG_BUF_SIZE];
  char heapInfo[DIAG_BUF_SIZE];
  char diag1[DIAG_BUF_SIZE];
  char nvmInfo[DIAG_BUF_LONG];

  char fbState[DIAG_BUF_SIZE];
  char fbError[64];
  char fbTimeout[DIAG_BUF_SIZE];

  void setValue(char* dest, const char* src) {
    if (src) {
        strncpy(dest, src, DIAG_BUF_SIZE - 1);
        dest[DIAG_BUF_SIZE - 1] = '\0';
    } else {
        dest[0] = ' ';
        dest[1] = '\0';
    }
  }

  void setValueLong(char* dest, const char* src) {
      if (src) {
          strncpy(dest, src, DIAG_BUF_LONG - 1);
          dest[DIAG_BUF_LONG - 1] = '\0';
      } else {
          dest[0] = ' ';
          dest[1] = '\0';
      }
  }

  void setDefault(char* dest) {
      dest[0] = '\0';
  }
};

extern Diag diagState;

