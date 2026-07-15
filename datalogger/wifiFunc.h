#pragma once

#define EDGE_HOST_ID 186
#define TEST_SERVER_HOST_ID 192
// #define SUBNET_ID 50
#define USE_STATIC_IP 0

// WIFI MODES
#define WIFI_RADIO_ON 1
#define WIFI_RADIO_OFF 0

void scanWifi();
void connectWifi();
void disconnectWifi();
bool ensureServerStarted();
bool waitForWifiStable(uint32_t stableMs = 1500, uint32_t timeoutMs = 20000);
bool wifiOK();
bool dnsOK();
bool internetOK();
bool internetOK443();
bool timeValid();
void updateWifiDiagState();

// Helpers
bool dnsReady();
bool wifiLinkReady();
bool wifiRadioOn();

// getters
const char* getLocalIP();




