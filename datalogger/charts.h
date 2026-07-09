#pragma once

#include <WiFi.h>

void renderGallonsPerDayChart(WiFiClient &client);
void renderCurrentWaveChart(WiFiClient &client);
void renderPumpEvent24hChart(WiFiClient &client);