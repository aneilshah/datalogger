// Library Files
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "global.h"

// Project Files
#include "button.h"
#include "mode.h"
#include "ntp.h"
#include "oled.h"
#include "power.h"
#include "wifiFunc.h"

// Just in case
#include <stdio.h>
#include <string.h>

// Global Variables
#include "global.h"

// Display
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

// OLED State
static uint8_t oledMode = OLED_OFF;

// Main Screen State
static uint32_t mainTimeout = OLED_MODE_NO_TIMEOUT;
static uint32_t oledModeTimer = 0;

// Modal State
static char modalTitle[32] = "";
static bool oledModalEvent = false;
static uint16_t countdown = 0;

// Popup State
static uint16_t popupTimeout = OLED_MODE_NO_TIMEOUT;
static char PopupText[32] = "";
static char PopupDetails[64] = "";
static uint8_t nextOledMode = OLED_MAIN;

// Screen Offsets
static unsigned int horOffset = 0;
static unsigned int vertOffset = 0;
static unsigned int vertOffsetMain = 0;

// Screen Data
constexpr uint8_t OLED_MAX_LINE_LEN = 48;
enum Line { LINE_0, LINE_1, LINE_2, LINE_3, LINE_4, LINE_5, LINE_COUNT};

static int drawX;
static int drawY;
static constexpr uint8_t lineY[] = { 0, 10, 20, 30, 40, 50};
static char lineBuffer[LINE_COUNT][OLED_MAX_LINE_LEN];

#define MIN_MODE_CYCLE_TIME (5 * LOOPS_PER_SEC)

// TODO:
// Consider changing screenTimer to uint16_t instead of uint32_t.
// Allow it to wrap (or reset) after ~10 hours (36000).
// All OLED timeouts are expected to be much shorter, so rollover
// will never affect normal operation while reducing memory usage.
// Keep OLED_MODE_NO_TIMEOUT (0xFFFF) as the "no timeout" sentinel.


//-------------------------------------------
// HELPERS
//-------------------------------------------

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setOledMode(uint8_t mode) {
  oledMode = mode;
  oledModeTimer = 0;
}

uint8_t getOledMode() {
  return oledMode;
}

void clearModalEvent() {
  oledModalEvent = false;
}

bool modalEvent() {
  return oledModalEvent;
}

void initDisplay() {
  display.init();
  setOledMode(OLED_OFF);
  VextON();
}

static void setScreenOrigin(int x, int y)
{
  drawX = x;
  drawY = y;
}

static void drawLine(Line line)
{
  display.drawString(drawX, drawY + lineY[line], lineBuffer[line]);
}

static void updateOLEDMode()
{
  switch (oledMode)
  {
    case OLED_MAIN:
      if (mainTimeout != OLED_MODE_NO_TIMEOUT && getScreenTimer() >= mainTimeout)
      {
        setOledMode(OLED_MINIMIZED);
      }
      break;

    case OLED_POPUP:
      if (getScreenTimer() >= popupTimeout)
      {
        setOledMode(nextOledMode);
      }
      break;

    case OLED_MODAL:
      if (buttonHold() < OLED_HOLD_TIMEOUT)
        countdown = OLED_HOLD_TIMEOUT - buttonHold();
      else
        countdown = 0;

      if (countdown == 0) {
        if (!oledModalEvent)
          Serial.println("MODAL EVENT");
        oledModalEvent = true;
      }
      break;

    default:
      break;
  }
}

// Main Screen Helpers
static void updateMainHeader(Line line)
{
  char suffix[3] = "";
  if (TEST_MODE) { strcpy(suffix, " T"); }

  snprintf(lineBuffer[line],
    sizeof(lineBuffer[line]),
    "DATA LOG %s%s",
    APP_VERSION,
    suffix);

  drawLine(line);
}

static void updateMainStatus(Line line)
{
  const auto &ramHeader = Logger.getRamHeader();
  const LoggerMode mode = getLoggerMode();
  const char* modeText = getLoggerModeTxt();

  if (mode == LoggerMode::RESET)
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]), "READY [NO DATA]");
  else if (mode == LoggerMode::LOGGING || mode == LoggerMode::PAUSED || mode == LoggerMode::STOPPED)
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
      "%s [%.1f Hr]", modeText, ramHeader.hoursStored);
  else
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]), "TBD MODE");

  drawLine(line);
}

static void updateMainCounts(Line line)
{
  const auto &session = Logger.getSessionStatistics();
  const auto &hour    = Logger.getHourStatistics();

  snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
    "#Ev  Hr: %u  Tot: %u", hour.count, session.count);

  drawLine(line);
}

static void updateMainLastEvent(Line line)
{
  const char* time = "---";
  snprintf(lineBuffer[line], sizeof(lineBuffer[line]), "Last Event: %s", time);
  drawLine(line);
}

// Revolving Line
static void updateMainInfo(Line line)
{
  /////////////////////////////////////////
  // Revolving Line
  /////////////////////////////////////////

  // Act:2h18m H:8  |  Avg:58s Max:14m  |  Min:3s Hr:18m  |  Samp:28452
  const auto &ramHeader = Logger.getRamHeader();
  const auto &session   = Logger.getSessionStatistics();

  const float TimePerStage = 2.0f;
  const int Stages = 3;
  const int TotalTime = LOOPS_PER_SEC * TimePerStage * Stages;
  const int TimePerStageLoops = int(TimePerStage * LOOPS_PER_SEC);

  if (LOOP_COUNT % TotalTime < TimePerStageLoops)
  {
    const char* dns = getLocalIP();
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]), "DNS: %s", dns);
  }
  else if (LOOP_COUNT % TotalTime < 2 * TimePerStageLoops)
  {
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
     "Min: %us   Max: %us", session.shortest, session.longest);
  }
  else
  {
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
      "Samples: %u", ramHeader.samplesTaken);
  }

  drawLine(line);
}

static void updateMainClock(Line line)
{
  float hours = LOOP_TIME / 36000.0f;
  if (hours < 24.0f)
  {
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
      "UP:%.1fh | CLK:%s", hours, getClock());
  }
  else
  {
    float days = hours / 24.0f;
    snprintf(lineBuffer[line], sizeof(lineBuffer[line]),
      "UP:%.1fd | CLK:%s", days, getClock());
  }

  drawLine(line);
}

// Main Screen
static void drawMain() {

  // Config Display
  setScreenOrigin(horOffset, vertOffsetMain);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // Draw the Lines
  updateMainHeader(LINE_0);       // Header
  updateMainStatus(LINE_1);       // Status
  updateMainCounts(LINE_2);       // Hour / Session Data
  updateMainLastEvent(LINE_3);    // Last Event
  updateMainInfo(LINE_4);         // Revolving Info Line
  updateMainClock(LINE_5);        // On Time / Clock
}

static void drawMinimized()
{
  const unsigned int CYCLE_COUNT = 8;
  unsigned int mode = (LOOP_TIME / MIN_MODE_CYCLE_TIME) % CYCLE_COUNT;

  // Config Display
  setScreenOrigin(horOffset, vertOffset);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  switch (mode)
  {
    case 0:
      snprintf(lineBuffer[LINE_0], sizeof(lineBuffer[LINE_0]),
        TEST_MODE ? "*TEST MODE* %s" : "LOGGER APP %s", APP_VERSION);
      drawLine(LINE_0);
      break;

    case 1:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "ON: %us [%ud]",  (unsigned)(LOOP_TIME / LOOPS_PER_SEC),
          (unsigned)(LOOP_TIME / 864000));
      drawLine(LINE_2);
      break;

    case 2:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
      "#Ev  Hr: %u  Tot: %u", 0, 0);
      drawLine(LINE_2);
      break;

    case 3:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "CLOCK: %s", getClock());
      drawLine(LINE_2);
      break;

    case 4:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "DATE: %s", getDate());
      drawLine(LINE_2);
      break;

    case 5:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "YEAR: %s", getYearStr());
      drawLine(LINE_2);
      break;

    case 6:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "Wifi: %s", CONN_STATUS);
      drawLine(LINE_2);
      break;

    default:
      snprintf(lineBuffer[LINE_2], sizeof(lineBuffer[LINE_2]),
        "Press TOP Button");
      drawLine(LINE_2);
      break;
  }

  if (LOOP_TIME % (120 * LOOPS_PER_SEC) == 0)
    vertOffset = random(0, 50);
}


static void drawPopup() {
  // Config Display
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // Draw Screen
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, PopupText);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 16, PopupDetails);
}

static void drawModal()
{
  // Wait this long before showing the progress bar
  constexpr uint16_t BAR_DELAY = LOOPS_PER_SEC / 2;   // 0.5 seconds

  // Progress bar Geometry
  constexpr int BAR_X = 10;
  constexpr int BAR_Y = 38;
  constexpr int BAR_W = 108;
  constexpr int BAR_H = 10;

  // Configure screen
  setScreenOrigin(horOffset, vertOffsetMain);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // Title
  snprintf(lineBuffer[LINE_1], sizeof(lineBuffer[LINE_1]),
    "%s", modalTitle);
  drawLine(LINE_1);

  uint16_t held = buttonHold();

  // Idle / Short Press: show cancel instruction
  if (held < BAR_DELAY)
  {
    snprintf(lineBuffer[LINE_4], sizeof(lineBuffer[LINE_4]),
      "Short Press to Cancel");
    drawLine(LINE_4);
    return;
  }

  // Remove the initial delay from the progress calculation
  held -= BAR_DELAY;

  uint16_t maxHold = OLED_HOLD_TIMEOUT - BAR_DELAY;
  if (held > maxHold)
    held = maxHold;

  // Draw Outline
  display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H);

  int fill = (held * (BAR_W - 2)) / OLED_HOLD_TIMEOUT;

  if (fill > 0)
  {
    display.fillRect(BAR_X + 1, BAR_Y + 1, fill, BAR_H - 2);
  }
}

void updateOLED() {
  updateOLEDMode();

  horOffset = (LOOP_TIME / (60 * LOOPS_PER_SEC)) % 10;  // Every 60 seconds, span = 10 pixels  

  // Clear The Display
  display.clear();

  switch (getOledMode())
  {
    case OLED_MAIN:
      drawMain();
      break;

    case OLED_MINIMIZED:
      drawMinimized();
      break;

    case OLED_POPUP:
      drawPopup();
      break;

    case OLED_MODAL:
      drawModal();
      break;
  }

  // Display the Screen
  display.display();
}

void showPopup(const char* text, const char* details, uint16_t timeout, uint8_t nextMode) {
  strncpy(PopupText, text, sizeof(PopupText) - 1);
  PopupText[sizeof(PopupText) - 1] = '\0';

  strncpy(PopupDetails, details, sizeof(PopupDetails) - 1);
  PopupDetails[sizeof(PopupDetails) - 1] = '\0';

  popupTimeout = timeout;
  nextOledMode = nextMode;
  setOledMode(OLED_POPUP);
}


void newPopupScreen(const char* text, const char* details, uint16_t timeout, uint8_t nextMode) {
  popupTimeout = timeout;
  nextOledMode = nextMode;
  showPopup(text, details, timeout, nextMode);
  updateOLED();
}

void oledMain(uint32_t duration) {
  setOledMode(OLED_MAIN);
  mainTimeout = duration;
}

void oledMinimized() {
  setOledMode(OLED_MINIMIZED);
}

void oledBlank() {
  setOledMode(OLED_OFF);
}

void oledOff() {
  display.displayOff();
}

void oledOn() {
  display.displayOn();
}

void oledModal(const char* title) {
  strncpy(modalTitle, title, sizeof(modalTitle) - 1);
  clearModalEvent();
  setOledMode(OLED_MODAL);
}

void oledPauseLogger() {
  setOledMode(OLED_PAUSE_LOGGER);
}
