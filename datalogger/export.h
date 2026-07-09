#pragma once

#include <WiFi.h>

// Main export entry point
void renderExportCsv(WiFiClient &client);
void renderExportJson(WiFiClient &client);


