//*****************************************************************************
// charts.cpp
// Logger Charts
//*****************************************************************************

#include "global.h"
#include "charts.h"

// Project Files
#include "bufferedPrint.h"
#include "loggerData.h"
#include "mode.h"

// Just in case
#include <stdio.h>

//*****************************************************************************
// Layout
//*****************************************************************************

static constexpr int W = 520;
static constexpr int H = 110;

static constexpr int PAD_LEFT   = 42;
static constexpr int PAD_RIGHT  = 12;
static constexpr int PAD_TOP    = 10;
static constexpr int PAD_BOTTOM = 24;
static constexpr int PLOT_W = W - PAD_LEFT - PAD_RIGHT;
static constexpr int PLOT_H = H - PAD_TOP - PAD_BOTTOM;

//*****************************************************************************
// Colors
//*****************************************************************************

static const char *BACKGROUND = "#fafafa";
static const char *GRID       = "#d0d0d0";
static const char *BAR        = "#2F6F73";

//*****************************************************************************
// Nice axis scaling
//*****************************************************************************

static uint32_t niceScale(uint32_t value)
{
  if (value <= 5)   return 5;
  if (value <= 10)  return 10;
  if (value <= 20)  return 20;
  if (value <= 50)  return 50;
  if (value <= 100) return 100;
  if (value <= 200) return 200;
  if (value <= 500) return 500;

  return ((value + 99) / 100) * 100;
}

//*****************************************************************************
// Forward declarations
//*****************************************************************************

static void renderLoggerSummary(Print &out);
static void renderLastHourChart(Print &out);
static void renderSessionChart(Print &out);


//*****************************************************************************
// Logger Summary
//*****************************************************************************

static void renderLoggerSummary(Print &out)
{
  const auto &ramHeader = Logger.getRamHeader();
  const auto &stats  = Logger.getSessionStatistics();

  out.println(F("<div class=\"chart-wrap\">"));

  out.println(
      F("<div class=\"chart-title\">"
        "Logger Summary"
        "</div>"));

  out.println(F("<table class=\"summary\">"));

  out.print(F("<tr><td>Started</td><td>"));
  out.print(ramHeader.startTime);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Stopped</td><td>"));
  out.print(ramHeader.stopTime);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Hours Logged</td><td>"));
  out.print(ramHeader.hoursStored);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Samples</td><td>"));
  out.print(ramHeader.samplesTaken);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Total Events</td><td>"));
  out.print(stats.count);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Active Seconds</td><td>"));
  out.print(stats.total);
  out.println(F("</td></tr>"));

  out.print(F("<tr><td>Shortest</td><td>"));
  out.print(stats.shortest);
  out.println(F(" sec</td></tr>"));

  out.print(F("<tr><td>Longest</td><td>"));
  out.print(stats.longest);
  out.println(F(" sec</td></tr>"));

  out.print(F("<tr><td>Events Detected</td><td>"));

  if (Logger.hasEvents())
    out.print(F("YES"));
  else
    out.print(F("NO"));

  out.println(F("</td></tr>"));
  out.println(F("</table>"));
  out.println(F("</div>"));
}

//*****************************************************************************
// Last Hour Chart
//*****************************************************************************

static void renderLastHourChart(Print &out)
{
  const auto &hour = Logger.getHourRecord();

  out.println(
    F("<div class=\"chart-wrap\">"
      "<div class=\"chart-title\">"
      "Last Hour Event History"
      "</div>"));

  //---------------------------------------------------------
  // Find maximum event count
  //---------------------------------------------------------

  uint32_t maxValue = 0;

  for (uint8_t i = 0; i < 60; i++)
  {
    if (hour.minute[i].count > maxValue)
      maxValue = hour.minute[i].count;
  }

  maxValue = niceScale(maxValue);

  //---------------------------------------------------------
  // SVG
  //---------------------------------------------------------

  out.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  out.print(W);
  out.print(' ');
  out.print(H);
  out.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  //---------------------------------------------------------
  // Background
  //---------------------------------------------------------

  out.print(F("<rect x=\""));
  out.print(PAD_LEFT);
  out.print(F("\" y=\""));
  out.print(PAD_TOP);
  out.print(F("\" width=\""));
  out.print(PLOT_W);
  out.print(F("\" height=\""));
  out.print(PLOT_H);
  out.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\"/>"));

  //---------------------------------------------------------
  // Horizontal grid
  //---------------------------------------------------------

  for (uint8_t i = 0; i <= 5; i++)
  {
  int y =
    PAD_TOP +
    PLOT_H -
    (PLOT_H * i / 5);

    out.print(F("<line x1=\""));
    out.print(PAD_LEFT);
    out.print(F("\" y1=\""));
    out.print(y);
    out.print(F("\" x2=\""));
    out.print(PAD_LEFT + PLOT_W);
    out.print(F("\" y2=\""));
    out.print(y);
    out.println(F("\" stroke=\"#d0d0d0\"/>"));
    out.print(F("<text text-anchor=\"end\" x=\""));
    out.print(PAD_LEFT - 5);
    out.print(F("\" y=\""));
    out.print(y + 4);
    out.print(F("\">"));
    out.print((maxValue * i) / 5);
    out.println(F("</text>"));
  }

  //---------------------------------------------------------
  // Bars
  //---------------------------------------------------------

  const float spacing =
      (float)PLOT_W / 60.0f;

  float width = spacing * 0.70f;

  if (width < 1.0f)
      width = 1.0f;

  if (width > 8.0f)
      width = 8.0f;

  for (uint8_t i = 0; i < 60; i++)
  {
      uint32_t value = hour.minute[i].count;
      float fraction = 0.0f;

      if (maxValue) fraction = (float)value / (float)maxValue;

      int barHeight = (int)(fraction * PLOT_H);
      int x = PAD_LEFT + (int)(i * spacing);
      int y = PAD_TOP + PLOT_H - barHeight;

      out.print(F("<rect x=\""));
      out.print(x);

      out.print(F("\" y=\""));
      out.print(y);

      out.print(F("\" width=\""));
      out.print((int)width);

      out.print(F("\" height=\""));
      out.print(barHeight);

      out.print(F("\" fill=\"#2F6F73\">"));
      out.print(F("<title>Minute "));
      out.print(i);
      out.print(F("&#10;Events: "));
      out.print(value);

      out.print(F("&#10;Shortest: "));
      out.print(hour.minute[i].shortest);

      out.print(F(" sec&#10;Longest: "));
      out.print(hour.minute[i].longest);

      out.print(F(" sec&#10;Active: "));
      out.print(hour.minute[i].total);

      out.println(F(" sec</title></rect>"));
  }

  //---------------------------------------------------------
  // Minute labels
  //---------------------------------------------------------

  for (uint8_t i = 0; i < 60; i += 5)
  {
    int x = PAD_LEFT + (int)(i * spacing);

    out.print(F("<text text-anchor=\"middle\" x=\""));
    out.print(x);
    out.print(F("\" y=\""));
    out.print(H - 5);
    out.print(F("\">"));

    if (i < 10)
      out.print('0');

    out.print(i);
    out.println(F("</text>"));
  }

  out.println(F("</svg>"));
  out.println(F("</div>"));
}

//*****************************************************************************
// Session Chart
//*****************************************************************************

static void renderSessionChart(Print &out)
{
  out.println(
    F("<div class=\"chart-wrap\">"
      "<div class=\"chart-title\">"
      "Full Session Event History"
      "</div>"));

  //---------------------------------------------------------
  // Read all completed hours
  //---------------------------------------------------------

  uint32_t values[LOGGER_MAX_HOURS];
  uint16_t hours = 0;
  uint32_t maxValue = 0;

  EventLogger::HourRecord hour;
  EventLogger::LogHeader ramHeader;

  if (!loggerDataReadNvmHeader(ramHeader))
  {
    out.println(F("<div class=\"small\">No logger data.</div></div>"));
    return;
  }

  Serial.printf("Header Hours Stored: %u\n", ramHeader.hoursStored);

  for (; hours < ramHeader.hoursStored; hours++)
  {
    Serial.printf("Reading Hour %u... ", hours);

    if (!loggerDataReadHourBlock(hours, hour))
    {
      Serial.println("FAILED");
      break;
    }

    uint32_t total = 0;

    for (uint8_t m = 0; m < 60; m++)
      total += hour.minute[m].count;

    values[hours] = total;

    if (total > maxValue)
      maxValue = total;

    Serial.printf("Events=%lu\n", total);
  }

  Serial.printf("Chart Hours=%u  Max=%lu\n",
    hours,
    maxValue);

  //---------------------------------------------------------
  // Add current hour (if logging)
  //---------------------------------------------------------

  const auto &current = Logger.getHourRecord();

  uint32_t total = 0;

  for (uint8_t m = 0; m < 60; m++)
  {
      total += current.minute[m].count;
  }

  if (total || hours == 0)
  {
      values[hours] = total;

      if (total > maxValue)
          maxValue = total;

      hours++;
  }

  //---------------------------------------------------------
  // Nothing to display
  //---------------------------------------------------------

  if (hours == 0)
  {
    out.println(
      F("<div class=\"small\">"
        "No logger history."
        "</div></div>"));

    return;
  }

  maxValue = niceScale(maxValue);

  //---------------------------------------------------------
  // SVG
  //---------------------------------------------------------

  out.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  out.print(W);
  out.print(' ');
  out.print(H);
  out.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  //---------------------------------------------------------
  // Background
  //---------------------------------------------------------

  out.print(F("<rect x=\""));
  out.print(PAD_LEFT);
  out.print(F("\" y=\""));
  out.print(PAD_TOP);
  out.print(F("\" width=\""));
  out.print(PLOT_W);
  out.print(F("\" height=\""));
  out.print(PLOT_H);
  out.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\"/>"));

  //---------------------------------------------------------
  // Grid
  //---------------------------------------------------------

  for (uint8_t i = 0; i <= 5; i++)
  {
    int y = PAD_TOP + PLOT_H - (PLOT_H * i / 5);

    out.print(F("<line x1=\""));
    out.print(PAD_LEFT);
    out.print(F("\" y1=\""));
    out.print(y);
    out.print(F("\" x2=\""));
    out.print(PAD_LEFT + PLOT_W);
    out.print(F("\" y2=\""));
    out.print(y);
    out.println(F("\" stroke=\"#d0d0d0\"/>"));

    out.print(F("<text text-anchor=\"end\" x=\""));
    out.print(PAD_LEFT - 5);
    out.print(F("\" y=\""));
    out.print(y + 4);
    out.print(F("\">"));
    out.print((maxValue * i) / 5);
    out.println(F("</text>"));
  }

  //---------------------------------------------------------
  // Bars
  //---------------------------------------------------------

  float spacing = (float)PLOT_W / (float)hours;
  float width = spacing * 0.70f;

  if (width < 1.0f)
    width = 1.0f;

  if (width > 8.0f)
    width = 8.0f;

  for (uint16_t i = 0; i < hours; i++) {
    float fraction = 0.0f;

    if (maxValue)
      fraction = (float)values[i] / (float)maxValue;

    int barHeight =
      (int)(fraction * PLOT_H);

    int x =
      PAD_LEFT +
      (int)(i * spacing);

    int y =
      PAD_TOP +
      PLOT_H -
      barHeight;

    out.print(F("<rect x=\""));
    out.print(x);
    out.print(F("\" y=\""));
    out.print(y);
    out.print(F("\" width=\""));
    out.print((int)width);
    out.print(F("\" height=\""));
    out.print(barHeight);
    out.print(F("\" fill=\"#2F6F73\">"));

    out.print(F("<title>Hour "));
    out.print(i + 1);
    out.print(F("&#10;Events: "));
    out.print(values[i]);
    out.println(F("</title></rect>"));
  }

  //---------------------------------------------------------
  // Day Labels
  //---------------------------------------------------------

  for (uint16_t i = 0; i < hours; i += 24)
  {
    int x = PAD_LEFT + (int)(i * spacing);

    out.print(F("<text text-anchor=\"middle\" x=\""));
    out.print(x);
    out.print(F("\" y=\""));
    out.print(H - 5);
    out.print(F("\">D"));
    out.print((i / 24) + 1);
    out.println(F("</text>"));
  }

  out.println(F("</svg>"));
  out.println(F("</div>"));
}

//*****************************************************************************
// Entry Point
//*****************************************************************************

void renderLoggerCharts(Print &out)
{
  //renderLoggerSummary(out);
  renderLastHourChart(out);
  renderSessionChart(out);
}