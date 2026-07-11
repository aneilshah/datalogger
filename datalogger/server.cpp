#include "global.h"

#include "charts.h"
#include "diag.h"
#include "export.h"
#include "pumpData.h"
#include "ntp.h"
#include "nvm.h"
#include "pumpFunc.h"
#include "utils.h"

extern uint16_t ADAPTIVE_DELAY;

// Create Wifi Server
WiFiServer server(80);  // Set web server port number to 80

// -------------------------
// Pages 
// -------------------------
static const uint8_t PAGE_LOGGER  = 0;
static const uint8_t PAGE_SYSTEMS = 1;
static const uint8_t PAGE_CHARTS  = 2;
static const uint8_t PAGE_CONN    = 3;
static const uint8_t PAGE_UTIL    = 4;
static const uint8_t PAGE_WIFI    = 7;

// -------------------------
// Constant HTML fragments in flash (saves RAM)
// -------------------------
static const char HTML_HEAD_A[] PROGMEM =
"<!DOCTYPE html><html>"
"<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<link rel=\"icon\" href=\"data:,\">";

static const char HTML_HEAD_SCRIPT[] PROGMEM =
"<script>"
"var curPage='pump';"
"function setPage(p){curPage=p;loadDoc(p);}"
"function loadDoc(p){"
"var xhttp=new XMLHttpRequest();"
"xhttp.onreadystatechange=function(){"
"if(this.readyState==4&&this.status==200){"
"document.getElementById('webpage').innerHTML=this.responseText;"
"}"
"};"
"xhttp.open('GET','/data?p='+encodeURIComponent(p),true);"
"xhttp.send();"
"}"
"function exportPage(p){"
"window.location='/export?p='+encodeURIComponent(p);"
"}"
"function exportJsonPage(p){"
"window.location='/json?p='+encodeURIComponent(p);"
"}"
//"setInterval(function(){loadDoc(curPage);},30000);"
"window.addEventListener('load',function(){loadDoc(curPage);});"
"</script>";

static const bool NAV_LEFT_MODE = true;   // true = left side nav, false = bottom nav

static const char HTML_HEAD_STYLE[] PROGMEM =
"<style>"
":root{"
  "--theme-color:#2F6F73;"
  "--theme-hover:#D9EEF0;"
  "--theme-font-size:18px;"
  "--theme-row-alt:#f5f7f7;"
  "--theme-measurement-color:#2a2a2a;"
  "--nav-left-gap:8px;"
  "--nav-width:110px;"
"}"

// headings
"h1,h2,h3{margin:10px 0;}"

// body
"body{margin:0;}"
"body{text-align:center;font-family:\"Arial\",Arial;font-size:var(--theme-font-size);}"

// Table
"table{width:80%;margin-left:auto;margin-right:auto;}"
"table{border-collapse:collapse;border-spacing:2px;background-color:white;border:4px solid var(--theme-color);}"
"th{padding:10px 14px;background-color:var(--theme-color);color:white;font-size:1.1em;}"
"tr{border:none;}"
"tr:nth-child(even){background-color:var(--theme-row-alt);}"
"tr:last-child{border-bottom:none;}"
"tr:hover{background-color:var(--theme-hover);}"
"td{padding:12px;}"
"td:first-child{font-weight:700;color:var(--theme-measurement-color);}"

// Page Wrap
".page-wrap{text-align:center;}"

// Title Wrap
".title-wrap{width:80%;margin-left:auto;margin-right:auto;text-align:center;}"
"body.nav-left .title-wrap{margin-left:0;margin-right:auto;}"

// Sensor Boxes
".sensor{display:inline-block;padding:6px 14px;border-radius:999px;background-color:var(--theme-color);color:white;font-weight:700;letter-spacing:0.3px;min-width:140px;text-align:center;box-shadow:0 1px 2px rgba(0,0,0,0.15);}"

// Sizes
".small{font-size:0.85em;}"
".large{font-size:1.25em;}"
".huge{font-size:1.5em;font-weight:bold;}"

/* Base nav/button styling */
".nav{display:flex;justify-content:center;gap:12px;flex-wrap:wrap;margin-top:12px;}"
".navbtn{min-width:100px;max-width:160px;flex:0 0 auto;padding:10px 12px;border-radius:10px;border:2px solid var(--theme-color);font-weight:700;font-size:1em;cursor:pointer;box-sizing:border-box;}"
".navbtn:active{transform:scale(0.98);box-shadow:0 1px 3px rgba(0,0,0,0.15);}"
".navbtn.active{background-color:#D9EEF0;color:#2F6F73;border-color:#2F6F73;}"
".navbtn.white{background-color:white;color:var(--theme-color);}"
".navbtn.theme{background-color:var(--theme-color);color:white;}"

/* Bottom nav mode */
"body.nav-bottom .nav{display:flex;justify-content:center;gap:12px;flex-wrap:wrap;margin-top:12px;}"
"body.nav-bottom .navbtn{min-width:100px;max-width:160px;flex:0 0 auto;}"

/* Left nav mode */
"body.nav-left{padding-left:calc(var(--nav-left-gap) + var(--nav-width) + var(--nav-left-gap));}"

"body.nav-left .nav{"
  "display:flex;"
  "flex-direction:column;"
  "align-items:stretch;"
  "position:fixed;"
  "left:8px;"
  "top:70px;"
  "width:110px;"
  "gap:8px;"
  "margin-top:0;"
  "z-index:1000;"
"}"

"body.nav-left .navbtn{"
  "width:100%;"
  "min-width:0;"
  "max-width:none;"
  "padding:8px 6px;"
  "font-size:0.82em;"
  "white-space:normal;"
  "line-height:1.1;"
  "text-align:center;"
"}"

"body.nav-left table{margin-left:0;margin-right:auto;}"

/* chart styling */
".chart-wrap{width:90%;max-width:1000px;margin:16px auto 28px auto;}"
".chart-title{font-weight:700;color:var(--theme-measurement-color);margin:4px 0 8px 0;}"
".chart{width:100%;height:auto;border:2px solid var(--theme-color);border-radius:14px;background:#fff;}"
".chart .grid{stroke:rgba(0,0,0,0.12);stroke-width:1;}"
".chart .line{fill:none;stroke:var(--theme-color);stroke-width:2;}"
".chart .pt{fill:var(--theme-color);} "
".chart text{font-family:Arial;font-size:12px;fill:#2a2a2a;}"

//------------------------------------------------------------------------
/* Phone */
"@media (max-width: 480px){"
  "table{width:95%;}"
  ".title-wrap{width:95%;}"
  ":root{--theme-font-size:16px;}"
  "td{padding:6px;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:4px 10px;min-width:120px;}"

  /* normal bottom mode */
  "body.nav-bottom .nav{gap:6px;}"
  "body.nav-bottom .navbtn{width:30%;min-width:0;max-width:none;padding:6px 6px;font-size:0.8em;border-radius:8px;box-sizing:border-box;white-space:normal;line-height:1.1;}"

  /* force left mode to behave like bottom mode on phone */
  "body.nav-left{padding-left:0;}"

  "body.nav-left .nav{"
    "position:static;"
    "flex-direction:row;"
    "justify-content:center;"
    "flex-wrap:wrap;"
    "width:auto;"
    "gap:6px;"
    "margin-top:12px;"
    "border-right:none;"
    "padding-right:0;"
    "left:auto;"
    "top:auto;"
  "}"

  "body.nav-left .navbtn{"
    "width:30%;"
    "min-width:0;"
    "max-width:none;"
    "padding:6px 6px;"
    "font-size:0.8em;"
    "border-radius:8px;"
    "box-sizing:border-box;"
    "white-space:normal;"
    "line-height:1.1;"
  "}"

  /* restore centered content on phone */
  "body.nav-left table{margin-left:auto;margin-right:auto;}"
  "body.nav-left .chart-wrap{margin-left:auto;margin-right:auto;}"
"}"

//------------------------------------------------------------------------
/* iPad */
"@media (min-width: 481px) and (max-width: 1024px){"
  "table{width:95%;}"
  ".title-wrap{width:95%;}"
  ":root{--theme-font-size:16px;}"
  "td{padding:6px;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:4px 10px;min-width:140px;}"

  "body.nav-bottom .nav{display:flex;justify-content:center;gap:10px;flex-wrap:nowrap;width:100%;max-width:700px;margin-left:auto;margin-right:auto;}"
  "body.nav-bottom .navbtn{flex:1 1 0;min-width:0;padding:8px 4px;font-size:0.9em;border-radius:8px;text-align:center;white-space:nowrap;}"

  "body.nav-left{padding-left:118px;}"
  "body.nav-left .nav{width:98px;left:8px;top:66px;gap:7px;}"
  "body.nav-left .navbtn{font-size:0.78em;padding:8px 5px;border-radius:8px;}"
"}"

//------------------------------------------------------------------------
/* Desktop */
"@media (min-width: 1025px){"
  "table{width:80%;}"
  ":root{--theme-font-size:20px;}"
  "td{padding:4px 8px;line-height:1.8;border-bottom:1px solid rgba(0,0,0,0.08);}"
  ".sensor{padding:2px 10px;line-height:1.3;min-width:170px;}"

  "body.nav-bottom .nav{display:flex;justify-content:center;gap:14px;flex-wrap:nowrap;width:100%;max-width:920px;margin-left:auto;margin-right:auto;}"
  "body.nav-bottom .navbtn{flex:1 1 0;min-width:0;padding:6px 16px;font-size:1em;border-radius:10px;text-align:center;white-space:nowrap;}"

  "body.nav-left{padding-left:140px;}"
  "body.nav-left .nav{width:116px;left:10px;top:72px;gap:8px;}"
  "body.nav-left .navbtn{font-size:0.85em;padding:8px 6px;border-radius:10px;}"
"}"

"</style>"
"</head><body class=\"";
static const char LOGGER_TABLE_OPEN[] PROGMEM =
"<table><tr><th>LOGGER INFO</th><th>VALUE</th></tr>";

static const char SYSTEM_TABLE_OPEN[] PROGMEM =
"<table><tr><th>SYSTEM DATA</th><th>VALUE</th></tr>";

static const char CONN_TABLE_OPEN[] PROGMEM =
"<table><tr><th>CONNECTIVITY DATA</th><th>VALUE</th></tr>";

static const char UTIL_TABLE_OPEN[] PROGMEM =
"<table><tr><th>UTIL DATA</th><th>VALUE</th></tr>";

static const char WIFI_TABLE_OPEN[] PROGMEM =
"<table><tr><th>WIFI DATA</th><th>VALUE</th></tr>";

static const char HTML_TABLE_CLOSE[] PROGMEM =
"</table>";

static const char HTML_BOTTOM[] PROGMEM =
"</div></body></html>";




// -------------------------
// HTTP helpers
// -------------------------
static inline void httpOkHtml(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println("Cache-Control: no-store");
  client.println();
}

static inline void httpNotFound(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}

static inline void httpCsvAttachment(WiFiClient &client, const char *filename) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/csv");
  client.print("Content-Disposition: attachment; filename=\"");
  client.print(filename);
  client.println("\"");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
}

static inline void httpJsonAttachment(WiFiClient &client, const char *filename) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.print("Content-Disposition: attachment; filename=\"");
  client.print(filename);
  client.println("\"");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
}

// -------------------------
// Table helpers (streamed, minimal Strings)
// -------------------------
static inline void printRowOpen(WiFiClient &client, const __FlashStringHelper *label) {
  client.print(F("<tr><td>"));
  client.print(label);
  client.print(F("</td><td><span class=\"sensor\">"));
}

static inline void printRowClose(WiFiClient &client) {
  client.println(F("</span></td></tr>"));
}

template<typename T>
static inline void printRow(WiFiClient &client, const __FlashStringHelper *label, T value) {
  printRowOpen(client, label);
  client.print(value);
  printRowClose(client);
}

static inline void printRow(WiFiClient &client, const __FlashStringHelper *label, uint8_t value) {
  printRowOpen(client, label);
  client.print((unsigned int)value);  // force numeric
  printRowClose(client);
}

// Reads first request line without allocating big Strings; returns true if got a line.
static bool readRequestLine(WiFiClient &client, char *buf, size_t buflen, uint32_t timeoutMs = 1500) {
  uint32_t start = millis();
  size_t idx = 0;

  while (millis() - start < timeoutMs) {
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\n') {
        buf[idx] = '\0';
        return true;
      }
      if (c != '\r') {
        if (idx + 1 < buflen) buf[idx++] = c;
      }
    }
    delay(1);
  }
  buf[idx] = '\0';
  return idx > 0;
}

// Extract "p=" page parameter from path (supports /data?p=... and /export?p=...)
// Defaults to Logger.
static uint8_t parsePageParam(const char *path, size_t pathLen) {
  const char *q = (const char*)memchr(path, '?', pathLen);
  if (!q) return PAGE_LOGGER;

  const char *p = strstr(q, "p=");
  if (!p) return PAGE_LOGGER;
  p += 2;

  if (strncmp(p, "systems", 7) == 0) return PAGE_SYSTEMS;
  if (strncmp(p, "charts", 6) == 0)  return PAGE_CHARTS;
  if (strncmp(p, "conn", 4) == 0)    return PAGE_CONN;
  if (strncmp(p, "util", 5) == 0)    return PAGE_UTIL;
  if (strncmp(p, "wifi", 4) == 0)    return PAGE_WIFI; 

  return PAGE_LOGGER;
}

// Navigation Buttons 
// LOGGER / SYS / CON / UTIL / WIFI / CHART / EXPORT* / REFERSH*)
static void renderNavButtons(WiFiClient &client, uint8_t active) {
  client.println(F("<div class=\"nav\">"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_LOGGER ? "active" : "white");
  client.println(F("\" onclick=\"setPage('logger')\">LOGGER</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_SYSTEMS ? "active" : "white");
  client.println(F("\" onclick=\"setPage('systems')\">SYS</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_CONN ? "active" : "white");
  client.println(F("\" onclick=\"setPage('conn')\">CONN</button>"));

  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_UTIL ? "active" : "white");
  client.println(F("\" onclick=\"setPage('util')\">UTIL</button>"));


  client.print(F("<button class=\"navbtn "));
  client.print(active == PAGE_WIFI ? "active" : "white");
  client.println(F("\" onclick=\"setPage('wifi')\">WIFI</button>"));

  client.println(F("<button class=\"navbtn theme\" onclick=\"setPage('charts')\">CHART</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"exportPage(curPage)\">EXPORT</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"exportJsonPage(curPage)\">JSON</button>"));
  client.println(F("<button class=\"navbtn theme\" onclick=\"loadDoc(curPage)\">REFRESH</button>"));

  client.println(F("</div>"));
}

// -------------------------
// Renderers (dynamic /data)
// -------------------------
static void renderLoggerTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)LOGGER_TABLE_OPEN);

  const bool noNewDataYetToday = (CYCLE_TODAY == 0);
  const bool showNA = (gYesterdayWasZero && noNewDataYetToday);

  // Total Cycles
  printRow(client, F("Pump Cycles"), String(Pump.getPumpEventCount()));

    // Cycles per HR [Day] (hold over unless yesterday was zero)
  if (showNA) {
    printRow(client, F("Cycles per HR [Day]"), F("N/A"));
  } else if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Cycles per HR [Day]"), F("***"));
  } else {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;
    const float cph = 60.0f / deltaMin;
    const int cpd = (int)(60.0f * 25.0f / deltaMin);

    String s;
    s.reserve(24);
    s = String(cph, 2);
    s += " [";
    s += String(cpd);
    s += "]";
    printRow(client, F("Cycles per HR [Day]"), s);
  }

  // Gallons per Hour [Day] (hold over unless yesterday was zero)
  // Note using 5 Gallons per Cycle (Probably should make this consistent)
  if (showNA) {
    printRow(client, F("Gallons per Hour [Day]"), F("N/A"));
  } else if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Gallons per Hour [Day]"), F("***"));
  } else {
    float deltaMin = Pump.getAvgCycleMin();
    if (deltaMin < 0.1f) deltaMin = 5.0f;
    const int  gph = (int)(5.0f * (60.0f / deltaMin));
    const uint32_t gpd = (uint32_t)(5.0f * 60.0f * 24.0f / deltaMin);

    String s;
    s.reserve(28);
    s = String(gph);
    s += " GPH [";
    s += String(gpd);
    s += "]";
    printRow(client, F("Gallons per Hour [Day]"), s);
  }

  const float loopsPerSec = (float)LOOPS_PER_SEC;

  // Total pump ON time
  const float onMin  = (float)Pump.getPumpOnTimeLoops() / (60.0f * loopsPerSec);
  const float onHour = (float)Pump.getPumpOnTimeLoops() / (3600.0f * loopsPerSec);
  if (onMin < 60.0f) printRow(client, F("TOTAL Pump ON Time"), String(onMin, 1) + "m");
  else               printRow(client, F("TOTAL Pump ON Time"), String(onHour, 2) + "hr");

  // Last pump ON time (seconds)
  const float lastOnSec = (float)Pump.getLastPumpOnTimeLoops() / loopsPerSec;
  printRow(client, F("Last Pump ON Time"), String(lastOnSec, 1) + "s");

  // Last cycle time
  if (Pump.getPumpEventCount() < 2) printRow(client, F("Last Cycle Time"), F("***"));
  else                              printRow(client, F("Last Cycle Time"), String(Pump.getDeltaMin(), 2) + "m");

  // Time since last cycle
  if (Pump.getPumpEventCount() < 1) {
    printRow(client, F("Time Since Last Cycle"), F("***"));
  } else {
    const float offTimeMin = (float)(LOOP_COUNT - Pump.getLastPumpEventLoops()) / (60.0f * loopsPerSec);
    printRow(client, F("Time Since Last Cycle"), String(offTimeMin, 2) + "m");
  }

  // Avg cycle + stdev
  if (Pump.getPumpEventCount() < 2) {
    printRow(client, F("Avg Cycle Time [StDev]"), F("***"));
  } else {
    String s;
    s.reserve(40);
    s = String(Pump.getAvgCycleMin(), 2);
    s += "m [";
    s += String(Pump.getStDevCycleMin(), 2);
    s += "]";
    printRow(client, F("Avg Cycle Time [StDev]"), s);
  } 

  // Current (steady-state from last completed event) + Energy in brackets
  if (Pump.getPumpEventCount() < 1) {
    printRow(client, F("Pump Current [Energy]"), F("***"));
  } else {
    String s;
    s.reserve(32);
    s = Pump.getLastEventSSCurText();               // e.g. "9.3A"
    s += " [";
    s += String(Pump.getLastCycleEnergyAmpSeconds(), 1);  // e.g. "142.6"
    s += " A·s]";

    printRow(client, F("Pump Current [Energy]"), s);
  }

  // Wifi Status
  printRow(client, F("Wifi Status"), CONN_STATUS);


  // Timestamp
  printRow(client, F("Timestamp"), getTimestamp());

  // Monitor time
  const float monMin = (float)LOOP_COUNT / (60.0f * loopsPerSec);
  const float monHr  = (float)LOOP_COUNT / (3600.0f * loopsPerSec);
  const float monDay = (float)LOOP_COUNT / (86400.0f * loopsPerSec);

  if (monHr < 1.0f)       printRow(client, F("Total Monitor Time"), String(monMin, 1) + "m");
  else if (monDay < 1.0f) printRow(client, F("Total Monitor Time"), String(monHr, 2) + "hr");
  else                    printRow(client, F("Total Monitor Time"), String(monDay, 2) + "d");

  client.print((const __FlashStringHelper*)HTML_TABLE_CLOSE);
  renderNavButtons(client, PAGE_LOGGER);
}

static void renderSystemTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)SYSTEM_TABLE_OPEN);

  printRow(client, F("Loop Count (100ms)"), String(LOOP_COUNT));
  printRow(client, F("Last-cycle Current (n | avg)"), Pump.getLastCycleCurrentSummaryText());
  printRow(client, F("Last Cycle Energy"), String(Pump.getLastCycleEnergyAmpSeconds(), 2) + " A·s");


  printRow(client, F("1 Sec Run Time"), String(LOOP_TIME) + " ms");
  printRow(client, F("Adaptive Loop Delay"), String(ADAPTIVE_DELAY) + " ms");
  printRow(client, F("Daily Totals Stored"), String(Pump.Daily365.dailyValidCount()) + " days");

  client.print((const __FlashStringHelper*)HTML_TABLE_CLOSE);

  // Nav Buttons
  renderNavButtons(client, PAGE_SYSTEMS);
}


static void renderConnTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)CONN_TABLE_OPEN);

  // Nav Buttons
  renderNavButtons(client, PAGE_CONN);
}

static void renderUtilTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)UTIL_TABLE_OPEN);


  // Nav Buttons
  renderNavButtons(client, PAGE_UTIL);
}



static void renderWifiTable(WiFiClient &client) {
  client.print((const __FlashStringHelper*)WIFI_TABLE_OPEN);

  printRow(client, F("WiFi State"), diagState.getWifiState());
  printRow(client, F("WiFi RSSI"), diagState.getWifiRSSI());
  printRow(client, F("WiFi BSSID"), diagState.getWifiBSSID());
  printRow(client, F("WiFi GW"), diagState.getWifiGW());
  printRow(client, F("WiFi IP"), diagState.getWifiIP());
  printRow(client, F("WiFi DNS"), diagState.getWifiDNS());
  printRow(client, F("WiFi Errors"), String(WIFI_ERR));

  // Nav Buttons
  renderNavButtons(client, PAGE_WIFI);
}

static void renderChartsPage(WiFiClient &client) {

  client.println(F("<h3>Charts</h3>"));
  // Charts
  renderLoggerChart(client);


  // Nav Buttons
  renderNavButtons(client, PAGE_CHARTS);
}

static void renderData(WiFiClient &client, uint8_t page) {
  switch (page) {
    case PAGE_SYSTEMS: renderSystemTable(client);  break;
    case PAGE_CHARTS:  renderChartsPage(client);   break;
    case PAGE_CONN:    renderConnTable(client);    break;
    case PAGE_UTIL:    renderUtilTable(client);   break;
    case PAGE_WIFI:    renderWifiTable(client);    break;
    case PAGE_LOGGER:
    default:           renderLoggerTable(client);    break;
  }
}


// -------------------------
// Shell (served at "/")
// -------------------------
static void renderPageShell(WiFiClient &client) {
  client.print((const __FlashStringHelper*)HTML_HEAD_A);
  client.print((const __FlashStringHelper*)HTML_HEAD_SCRIPT);
  client.print((const __FlashStringHelper*)HTML_HEAD_STYLE);
  client.print(NAV_LEFT_MODE ? "nav-left" : "nav-bottom");
  client.println(F("\">"));

  client.println(F("<div class=\"page-wrap\">"));
  client.println(F("<div class=\"title-wrap\">"));
  client.print(F("<h3>Shahman Data Logger "));
  client.print(APP_VERSION);
  if (TEST_MODE) client.print(F(" *TEST*"));
  client.println(F("</h3>"));
  client.println(F("</div>"));

  client.println(F("<div id=\"webpage\"></div>"));

  client.println(F("</div>"));

  client.print((const __FlashStringHelper*)HTML_BOTTOM);
}

// -------------------------
// Main entry
// -------------------------
void webServer() {
  WiFiClient client = server.available();
  client.setNoDelay(true);
  if (!client) return;

  char line[180];
  if (!readRequestLine(client, line, sizeof(line))) {
    client.stop();
    return;
  }

  const bool isGet = (strncmp(line, "GET ", 4) == 0);
  const char *path = isGet ? (line + 4) : nullptr;

  bool isRoot = false;
  bool isData = false;
  bool isExport = false;
  bool isJson = false;
  uint8_t page = PAGE_LOGGER;

  if (path && path[0] == '/') {
    const char *sp = strchr(path, ' ');
    size_t len = 0;
    if (sp) len = (size_t)(sp - path);
    else    len = strlen(path);

    isRoot = (len == 1);

    if (len >= 5 && strncmp(path, "/data", 5) == 0) {
      isData = true;
      page = parsePageParam(path, len);
    }

    if (len >= 7 && strncmp(path, "/export", 7) == 0) {
      isExport = true;
      page = parsePageParam(path, len);
    }

    if (len >= 5 && strncmp(path, "/json", 5) == 0) {
      isJson = true;
      page = parsePageParam(path, len);
    }
  }

  if (!isGet || (!isRoot && !isData && !isExport && !isJson)) {
    httpNotFound(client);
    client.stop();
    return;
  }

  if (isExport) {
    char fname[48];
    char ts[32];
    snprintf(ts, sizeof(ts), "%s", getTimestamp());


    // Export Data File
    for (int i = 0; ts[i]; i++) {
      if (ts[i] == ':' || ts[i] == ' ') ts[i] = '_';
    }

    char suffix[10]; 
    if (TEST_MODE) snprintf(suffix, sizeof(suffix), "_test");
    else snprintf(suffix, sizeof(suffix), "");  
    snprintf(fname, sizeof(fname), "pump%s_export_%s.csv", suffix, ts);

    httpCsvAttachment(client, fname);
    renderExportCsv(client);
  } 
  
  // Export JSON
  else if (isJson) {
    char fname[48];
    char ts[32];
    snprintf(ts, sizeof(ts), "%s", getTimestamp());


    // Export JSON File
    for (int i = 0; ts[i]; i++) {
      if (ts[i] == ':' || ts[i] == ' ') ts[i] = '_';
    }

    char suffix[10]; 
    if (TEST_MODE) snprintf(suffix, sizeof(suffix), "_test");
    else snprintf(suffix, sizeof(suffix), "");  
    snprintf(fname, sizeof(fname), "logger%s_json_%s.json", suffix, ts);

    httpJsonAttachment(client, fname);
  } 

  else { 
    httpOkHtml(client);
    if (isData) renderData(client, page);
    else renderPageShell(client);
  }

  //client.flush();
  client.stop();
}