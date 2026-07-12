//*****************************************************************************
// charts.cpp
//
// Logger Charts
//
//*****************************************************************************

#include "charts.h"

#include "global.h"
#include "mode.h"

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

static constexpr int PLOT_W =
    W - PAD_LEFT - PAD_RIGHT;

static constexpr int PLOT_H =
    H - PAD_TOP - PAD_BOTTOM;

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

static void renderLoggerSummary(WiFiClient &client);
static void renderLastHourChart(WiFiClient &client);
static void renderSessionChart(WiFiClient &client);


//*****************************************************************************
// Logger Summary
//*****************************************************************************

static void renderLoggerSummary(WiFiClient &client)
{
    const auto &header = Logger.getHeader();
    const auto &stats  = Logger.getSessionStatistics();

    client.println(F("<div class=\"chart-wrap\">"));

    client.println(
        F("<div class=\"chart-title\">"
          "Logger Summary"
          "</div>"));

    client.println(F("<table class=\"summary\">"));

    client.print(F("<tr><td>Started</td><td>"));
    client.print(header.startTime);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Stopped</td><td>"));
    client.print(header.stopTime);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Hours Logged</td><td>"));
    client.print(header.hoursStored);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Samples</td><td>"));
    client.print(header.samplesTaken);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Total Events</td><td>"));
    client.print(stats.count);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Active Seconds</td><td>"));
    client.print(stats.total);
    client.println(F("</td></tr>"));

    client.print(F("<tr><td>Shortest</td><td>"));
    client.print(stats.shortest);
    client.println(F(" sec</td></tr>"));

    client.print(F("<tr><td>Longest</td><td>"));
    client.print(stats.longest);
    client.println(F(" sec</td></tr>"));

    client.print(F("<tr><td>Events Detected</td><td>"));

    if (Logger.hasEvents())
        client.print(F("YES"));
    else
        client.print(F("NO"));

    client.println(F("</td></tr>"));

    client.println(F("</table>"));

    client.println(F("</div>"));
}

//*****************************************************************************
// Last Hour Chart
//*****************************************************************************

static void renderLastHourChart(WiFiClient &client)
{
    const auto &hour = Logger.getHourRecord();

    client.println(
        F("<div class=\"chart-wrap\">"
          "<div class=\"chart-title\">"
          "Last Hour"
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

    client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
    client.print(W);
    client.print(' ');
    client.print(H);
    client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

    //---------------------------------------------------------
    // Background
    //---------------------------------------------------------

    client.print(F("<rect x=\""));
    client.print(PAD_LEFT);
    client.print(F("\" y=\""));
    client.print(PAD_TOP);
    client.print(F("\" width=\""));
    client.print(PLOT_W);
    client.print(F("\" height=\""));
    client.print(PLOT_H);
    client.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\"/>"));

    //---------------------------------------------------------
    // Horizontal grid
    //---------------------------------------------------------

    for (uint8_t i = 0; i <= 5; i++)
    {
        int y =
            PAD_TOP +
            PLOT_H -
            (PLOT_H * i / 5);

        client.print(F("<line x1=\""));
        client.print(PAD_LEFT);

        client.print(F("\" y1=\""));
        client.print(y);

        client.print(F("\" x2=\""));
        client.print(PAD_LEFT + PLOT_W);

        client.print(F("\" y2=\""));
        client.print(y);

        client.println(
            F("\" stroke=\"#d0d0d0\"/>"));

        client.print(F("<text text-anchor=\"end\" x=\""));

        client.print(PAD_LEFT - 5);

        client.print(F("\" y=\""));

        client.print(y + 4);

        client.print(F("\">"));

        client.print((maxValue * i) / 5);

        client.println(F("</text>"));
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
        uint32_t value =
            hour.minute[i].count;

        float fraction = 0.0f;

        if (maxValue)
            fraction =
                (float)value /
                (float)maxValue;

        int barHeight =
            (int)(fraction * PLOT_H);

        int x =
            PAD_LEFT +
            (int)(i * spacing);

        int y =
            PAD_TOP +
            PLOT_H -
            barHeight;

        client.print(F("<rect x=\""));
        client.print(x);

        client.print(F("\" y=\""));
        client.print(y);

        client.print(F("\" width=\""));
        client.print((int)width);

        client.print(F("\" height=\""));
        client.print(barHeight);

        client.print(F("\" fill=\"#2F6F73\">"));

        client.print(F("<title>Minute "));

        client.print(i);

        client.print(F("&#10;Events: "));

        client.print(value);

        client.print(F("&#10;Shortest: "));
        client.print(hour.minute[i].shortest);

        client.print(F(" sec&#10;Longest: "));
        client.print(hour.minute[i].longest);

        client.print(F(" sec&#10;Active: "));
        client.print(hour.minute[i].total);

        client.println(F(" sec</title></rect>"));
    }

    //---------------------------------------------------------
    // Minute labels
    //---------------------------------------------------------

    for (uint8_t i = 0; i < 60; i += 5)
    {
        int x =
            PAD_LEFT +
            (int)(i * spacing);

        client.print(F("<text text-anchor=\"middle\" x=\""));

        client.print(x);

        client.print(F("\" y=\""));

        client.print(H - 5);

        client.print(F("\">"));

        if (i < 10)
            client.print('0');

        client.print(i);

        client.println(F("</text>"));
    }

    client.println(F("</svg>"));

    client.println(F("</div>"));
}

//*****************************************************************************
// Session Chart
//*****************************************************************************

// Implemented by logger/flash module
bool loggerReadHour(
    uint16_t hourIndex,
    EventLogger::HourRecord &hour);

static void renderSessionChart(WiFiClient &client)
{
    client.println(
        F("<div class=\"chart-wrap\">"
          "<div class=\"chart-title\">"
          "Full Session"
          "</div>"));

    //---------------------------------------------------------
    // Read all completed hours
    //---------------------------------------------------------

    uint32_t values[336];

    uint16_t hours = 0;
    uint32_t maxValue = 0;

    EventLogger::HourRecord hour;

    while (hours < 336)
    {
        if (!loggerReadHour(hours, hour))
            break;

        uint32_t total = 0;

        for (uint8_t m = 0; m < 60; m++)
        {
            total += hour.minute[m].count;
        }

        values[hours] = total;

        if (total > maxValue)
            maxValue = total;

        hours++;
    }

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
        client.println(
            F("<div class=\"small\">"
              "No logger history."
              "</div></div>"));

        return;
    }

    maxValue = niceScale(maxValue);

    //---------------------------------------------------------
    // SVG
    //---------------------------------------------------------

    client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
    client.print(W);
    client.print(' ');
    client.print(H);
    client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

    //---------------------------------------------------------
    // Background
    //---------------------------------------------------------

    client.print(F("<rect x=\""));
    client.print(PAD_LEFT);
    client.print(F("\" y=\""));
    client.print(PAD_TOP);
    client.print(F("\" width=\""));
    client.print(PLOT_W);
    client.print(F("\" height=\""));
    client.print(PLOT_H);
    client.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\"/>"));

    //---------------------------------------------------------
    // Grid
    //---------------------------------------------------------

    for (uint8_t i = 0; i <= 5; i++)
    {
        int y =
            PAD_TOP +
            PLOT_H -
            (PLOT_H * i / 5);

        client.print(F("<line x1=\""));
        client.print(PAD_LEFT);
        client.print(F("\" y1=\""));
        client.print(y);
        client.print(F("\" x2=\""));
        client.print(PAD_LEFT + PLOT_W);
        client.print(F("\" y2=\""));
        client.print(y);
        client.println(F("\" stroke=\"#d0d0d0\"/>"));

        client.print(F("<text text-anchor=\"end\" x=\""));
        client.print(PAD_LEFT - 5);
        client.print(F("\" y=\""));
        client.print(y + 4);
        client.print(F("\">"));
        client.print((maxValue * i) / 5);
        client.println(F("</text>"));
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

    for (uint16_t i = 0; i < hours; i++)
    {
        float fraction = 0.0f;

        if (maxValue)
            fraction =
                (float)values[i] /
                (float)maxValue;

        int barHeight =
            (int)(fraction * PLOT_H);

        int x =
            PAD_LEFT +
            (int)(i * spacing);

        int y =
            PAD_TOP +
            PLOT_H -
            barHeight;

        client.print(F("<rect x=\""));
        client.print(x);
        client.print(F("\" y=\""));
        client.print(y);
        client.print(F("\" width=\""));
        client.print((int)width);
        client.print(F("\" height=\""));
        client.print(barHeight);
        client.print(F("\" fill=\"#2F6F73\">"));

        client.print(F("<title>Hour "));
        client.print(i + 1);
        client.print(F("&#10;Events: "));
        client.print(values[i]);
        client.println(F("</title></rect>"));
    }

    //---------------------------------------------------------
    // Day Labels
    //---------------------------------------------------------

    for (uint16_t i = 0; i < hours; i += 24)
    {
        int x =
            PAD_LEFT +
            (int)(i * spacing);

        client.print(F("<text text-anchor=\"middle\" x=\""));
        client.print(x);
        client.print(F("\" y=\""));
        client.print(H - 5);
        client.print(F("\">D"));
        client.print((i / 24) + 1);
        client.println(F("</text>"));
    }

    client.println(F("</svg>"));
    client.println(F("</div>"));
}

//*****************************************************************************
// Entry Point
//*****************************************************************************

void renderLoggerCharts(WiFiClient &client)
{
  renderLoggerSummary(client);
  renderLastHourChart(client);
  renderSessionChart(client);
}