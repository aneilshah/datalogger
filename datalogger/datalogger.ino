// Arduino Library Files
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

// My Libraries
#include "event.h"

// Global Include
#include "global.h"

// My Drivers
#include "button.h"
#include "mode.h"
#include "ntp.h"
#include "nvm.h"
#include "oled.h"
#include "ota.h"
#include "power.h"
#include "ramlog.h"
#include "server.h"
#include "wifiFunc.h"

// Project Files
#include "datastore.h"
#include "diag.h"
#include "ledFunc.h"
#include "logger.h"
#include "mode.h"
#include "pumpData.h"
#include "pumpFunc.h"
#include "testcode.h"
#include "test_mode.h"

// Version Info
const char APP_VERSION[] PROGMEM = "V0.0A";

// System States
const char* CONN_STATUS = "OFF";
uint32_t LOOP_COUNT = 0;
uint32_t LOOP_TIME = 1000;
int ISR_CNT = 0;
unsigned int ONE_SEC_TIMER_MS = 0;
uint16_t ADAPTIVE_DELAY = 98;
unsigned int START_TIME = 0;

// Error Counts
int WIFI_ERR = 0;

// Hardware States
int ADC1_COUNT = 0;
float ADC1_VOLT = 0;
int BTN_VAL = 0;
int D1_VAL = 2;
int TS1_VAL = 0;

// NVM Boot Status
static bool gNvmRestoreDone = false;

// Events
Event testEvent;

// Diagnostics
Diag diagState;

// Binary Identifier
#if TEST_MODE
__attribute__((used)) static const char BUILD_MODE_MARKER[] PROGMEM =
    "PUMP_MONITOR|MODE=TEST";
#else
__attribute__((used)) static const char BUILD_MODE_MARKER[] PROGMEM =
    "PUMP_MONITOR|MODE=PROD";
#endif

const char* getBuildModeMarker() {
  return BUILD_MODE_MARKER;
}

void IRAM_ATTR isrFunction() {
  PUMP_EVENT = 1;
  ISR_CNT++;
}

void initNvmBootRestore() {
  if (gNvmRestoreDone) return;

  if (!timeValid()) {
    Serial.println("Time not valid yet; skipping NVM restore.");
    return;
  }

  char dayKey[16]; 
  const uint32_t nowEpoch = getCurrentEpoch();
  getCurrentDayKey(dayKey, sizeof(dayKey));

  // Update boot stats
  nvmUpdateBootStats(nowEpoch);

  gNvmRestoreDone = true;  // if we got here the restore was completed
}

void setup() {
  Serial.begin(115200);

  // Init ADC
  adcAttachPin(ADC1_PIN);
  //analogSetClockDiv(255); // 1338mS

  // Init LED
  pinMode(LED_PIN, OUTPUT);  // Set GPIO25 as digital output pin

  // Init ISR
  //pinMode(ISR_PIN, INPUT_PULLUP);
  //attachInterrupt(ISR_PIN, isrFunction, RISING);

  // Init OLED
  initDisplay();

  // Init NVM
  nvmInit();

  // Randomize
  randomSeed((uint32_t)esp_random());

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  Serial.println();
  Serial.println();
  Serial.print(F("### STARTING DATA LOGGER: "));
  Serial.print(FPSTR(APP_VERSION));   // FPSTR reads PROGMEM string
  Serial.println(F(" ###"));
  Serial.println("Setup Complete");
  //scanWifi();   // DISABLED – creates issues with Firebase TLS stability.  Only use for debugging board bring up
  Serial.println(getBuildModeMarker());
  connectWifi();

  // NTP Time Server
  setupNTP();
  oledMain(MAIN_TIMEOUT_SEC);

  // Setup NVM Boot Restore  (must be after NTP)
  initNvmBootRestore();
  nvmDumpPumpState();

  // Init Pump
  initPump();
  initLogger();

  // Store Startup Time
  START_TIME = (millis() / 1000); 

  // Test Functions
  //testDataStore(); 

  // Init OTA
  initOTA(TEST_MODE ? "pump-test" : "pump-prod");

  // Setup done, go to main screen
  oledMain(MAIN_TIMEOUT_SEC);
}

void loop() {
  static unsigned long loopCount = 0;

  webServer();   // SERVICE HTTP AS FAST AS POSSIBLE

  loop100ms();                              // Run the 100ms loop
  if (loopCount % 10 == 0) loop1Sec();      // Run the 1 sec loop
  if (loopCount % 100 == 0) loop10Sec();    // Run the 10 sec loop
  if (loopCount % 600 == 0) loop1Min();     // Run the 1 minute loop

  // Complete the loop
  loopCount++;
  LOOP_COUNT++;

  // Do this in low power mode
  if (!wifiRadioOn()) {
    esp_sleep_enable_timer_wakeup(ADAPTIVE_DELAY * 1000);   // 98 ms
    esp_light_sleep_start();
    wakeUp();
  }

  // Do this in full power mode
  else {
    delay(ADAPTIVE_DELAY);  // avg code run time is ~2 ms
  }
}

void loop100ms() {
  // Run Simulation if needed
  if (TEST_MODE) simulateLogger();

  // Read Inputs
  readDigitalButton();
  readADC();

  // Run Main Logic
  checkForPumpEvent();  // Want accurate timing so check every 100ms
  processPumpEvent();
  processLoggerMode();

  // Update Outputs
  updateOLED();
  //updatePopupScreen();
  setLed();
  //webServer();  REMOVE AFRER TESTING

  // process Events
  processLoopCheck();
  processTestEvent();

  // Others
  handleOTA();
}

void loop1Sec() {
  static uint32_t count_1s;

  // Adaptive Loop Adjust
  static uint32_t prevLoopStart;
  uint32_t msNow = millis();
  ONE_SEC_TIMER_MS = msNow - prevLoopStart;
  prevLoopStart = msNow;

  // Dynamically Adjust Loop Time
  if (ONE_SEC_TIMER_MS < 1000 && ADAPTIVE_DELAY < 100) ADAPTIVE_DELAY++;
  else if (ONE_SEC_TIMER_MS > 1000 && ADAPTIVE_DELAY > 80) ADAPTIVE_DELAY--;


  // process 1 second tasks
  readDigital();
  readTouchSwitch();
  writeDAC();

  // Keep NTP Clock Valid
  if (wifiOK()) updateNTP();

  // update loop counter
  count_1s++;
}

void loop10Sec() {
  ensureServerStarted();

  // Early WiFi stabilization
  if (LOOP_COUNT < 40 * LOOPS_PER_SEC) checkWifi();


  // OTA
  void updateOTAState();

  // Make sure NVM was checked after boot
  if (!gNvmRestoreDone) initNvmBootRestore();  
}

void loop1Min() {
  static uint16_t minCount = 0;
  minCount = (minCount + 1) % 600;  // Prevent Overflow

  // Update System State
  if (WiFi.status() != WL_CONNECTED) diagState.setSystemState("*NO WIFI*");
  else if (!validClock()) diagState.setSystemState("*NO CLOCK*");
  else diagState.setSystemState("RUNNING");

  // Reconnect Wifi if needed
  checkWifi();
  updateWifiDiagState();
  diagState.updateDiagInfo();

  // Check NTP
  ensureTimeHealthy();

  // Run normal scheduled hourly tasks
  checkForOneHourTasks();
}

void checkForOneHourTasks() {
  static int8_t lastHour = -1;

  // Only run hourly logic if NTP/system time is valid
  if (!validClock()) return;

  const uint8_t hour = getHourInt();
  if (hour < 0 || hour > 23) return;

  // First valid hour after boot: initialize and do nothing
  if (lastHour < 0) {
    lastHour = hour;
    return;
  }

  // Detect a clean hour rollover (including 23 -> 0)
  const bool advanced =
    ((hour > 0 && lastHour == hour - 1) || (hour == 0 && lastHour == 23));

  if (hour != lastHour && advanced) {
    loop1Hour(hour);
    if (hour == 0) loop1DayMidnight();
    lastHour = hour;
  } 
  else if (hour != lastHour) {
    // Clock jumped (NTP correction / reboot / manual set) — resync without firing loops
    lastHour = hour;
  }
}

// One Hour Tasks - On the hour
void loop1Hour(int hour) {
  Serial.println();
  Serial.print("*** RUNNING 1 HOUR LOOP [");
  Serial.print(hour);
  Serial.println("]");

}

// Midnight
void loop1DayMidnight() {
  Serial.println();
  Serial.println("*** RUNNING 1 DAY LOOP - MIDNIGHT");

}

//------------------------------------------------------------------------

void writeDAC() {
  //dacWrite(DAC_PIN, LOOP_COUNT % 250);
}

void readADC() {
  uint32_t startTime = micros();

  if (TEST_MODE) ADC1_COUNT = getTestModeADC();
  else ADC1_COUNT = analogRead(ADC1_PIN);

  ADC1_VOLT = 3.3 * (ADC1_COUNT / 4095.0);
  TLog("ADC: ", ADC1_COUNT, startTime);
  TLog("ADC read time: ", startTime);
}

void readDigital() {
  uint32_t startTime = micros();
  D1_VAL = digitalRead(D1_PIN);
  TLog("Digital read time: ", startTime);
}

void readDigitalButton() {
  static uint8_t oldBtn = 1;
  static uint16_t holdTime = 0;
  const uint16_t MAX_HOLD = 50000;

  BTN_VAL = !digitalRead(BTN_PIN);     // Read New Button Status
  if (BTN_VAL) 
  {
    if (holdTime < MAX_HOLD) holdTime++;
  }
  else holdTime = 0;
  processButton(holdTime);
  return;

  ////////////////////////////////////////////////////

  // if (oldBtn == 1 && BTN_VAL == 0) {
  //   processButton(holdTime);
  //   holdTime = 0;
  // }
  // else if (oldBtn == 1 && BTN_VAL == 1) holdTime++;
  // else holdTime = 0;
  // oldBtn = BTN_VAL;                   // Save Status
}

void readTouchSwitch() {
  uint32_t startTime = micros();
  TS1_VAL = touchRead(TS1_PIN);
  TLog("TS read time: ", startTime);
}

void processLoopCheck() {
  static uint32_t loopCount = 0;
  static uint32_t startTime = 0;
  if (loopCount % 999 == 0) {
    LOOP_TIME = int((millis() - startTime) / 100);
    Serial.println();
    if (LOOP_TIME < 950 || LOOP_TIME > 1050) {
      Serial.println("*** Total time for 1000 loops:" + String(LOOP_TIME));
    }
    startTime = millis();
  }
  loopCount++;
}

void processTestEvent() {
  if (testEvent.check()) {
    Serial.println();
    Serial.println("*** TEST EVENT ***");
    Serial.println("Total time for Test Event:" + String(testEvent.getDelta()) + "ms");
    Serial.println();
    testEvent.setSec(73);
  }
}

void testDataStore() {
  delay(1000);
  DataStoreFloat X(15, 1, "abc");
  for (int i = 0; i <100; i++) {
    X.addData(i + (float(i)/100));
    Serial.println(X.dataText());
  }
}

void checkWifi() {
  if (!wifiRadioOn()) return; 

  if (!wifiOK()) {
    Serial.println("WiFi not OK, reconnecting...");
    connectWifi();
    WIFI_ERR++;
    return;
  }

  // 2) Optional: ensure WiFi stabilizes after reconnect
  if (!waitForWifiStable(1500, 5000)) {
    Serial.println("WiFi not stable yet");
    return;
  }
}

// Version History
// 0.0 Jul-09-2026 Initial Code
