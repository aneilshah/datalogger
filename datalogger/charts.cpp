#include "charts.h"

#include "global.h"
#include "eventData.h"
#include "ntp.h"
#include "utils.h"

namespace {

static void formatHourLabel(int hour24, char* out, size_t outLen) {
  const bool isPm = (hour24 >= 12);
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;
  snprintf(out, outLen, "%d%s", hour12, isPm ? "PM" : "AM");
}

}  // namespace



// -----------------------------------------------------------------------------
// Logger event 24h chart
// -----------------------------------------------------------------------------
void renderLoggerChart(WiFiClient &client) {
  eventDataPurgeNow();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Logger Events - Last 24 Hours</div>"));

  const int eventCount = eventDataCount();
  if (eventCount <= 0) {
    client.println(F("<div class=\"small\">No logger events.</div></div>"));
    return;
  }

  const uint32_t nowMin2026 = eventDataGetMinutesSince2026();
  const uint32_t minutesSinceBootNow = minutesSinceBoot();
  uint32_t windowMinutes = minutesSinceBootNow;
  if (windowMinutes > 24U * 60U) windowMinutes = 24U * 60U;
  if (windowMinutes < 60U) windowMinutes = 60U;

  uint32_t windowHours = (windowMinutes + 59U) / 60U;
  if (windowHours < 1U) windowHours = 1U;

  uint32_t labelEveryHours;
  if (windowHours <= 4U)       labelEveryHours = 1U;
  else if (windowHours <= 12U) labelEveryHours = 2U;
  else                         labelEveryHours = 4U;

  const int W = 520;
  const int H = 90;

  const int padL = 40;
  const int padR = 12;
  const int padT = 10;
  const int padB = 24;

  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  const int x0 = padL;
  const int x1 = padL + plotW;

  const int yLow  = padT + plotH - 6;
  const int yHigh = padT + 8;

  // ---- adaptive pulse width ----
  int pulseWidthPx = 2;
  if (eventCount > 0) {
    const float spacing = (float)plotW / (float)eventCount;
    float w = spacing * 0.10f;

    if (w < 1.0f)  w = 1.0f;
    if (w > 10.0f) w = 10.0f;

    pulseWidthPx = (int)(w + 0.5f);
  }

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  // background
  client.print(F("<rect x=\""));
  client.print(x0);
  client.print(F("\" y=\""));
  client.print(padT);
  client.print(F("\" width=\""));
  client.print(plotW);
  client.print(F("\" height=\""));
  client.print(plotH);
  client.println(F("\" fill=\"#fafafa\" stroke=\"#d0d0d0\" stroke-width=\"1\"/>"));

  // baseline
  client.print(F("<line class=\"grid\" x1=\""));
  client.print(x0);
  client.print(F("\" y1=\""));
  client.print(yLow);
  client.print(F("\" x2=\""));
  client.print(x1);
  client.print(F("\" y2=\""));
  client.print(yLow);
  client.println(F("\"/>"));

  // ON/OFF labels
  client.print(F("<text text-anchor=\"end\" x=\""));
  client.print(padL - 6);
  client.print(F("\" y=\""));
  client.print(yHigh + 4);
  client.println(F("\">ON</text>"));

  client.print(F("<text text-anchor=\"end\" x=\""));
  client.print(padL - 6);
  client.print(F("\" y=\""));
  client.print(yLow + 4);
  client.println(F("\">OFF</text>"));

  // time grid
  const time_t nowEpoch = (time_t)getCurrentEpoch();
  struct tm nowTm;
  localtime_r(&nowEpoch, &nowTm);

  for (int hAgo = (int)windowHours; hAgo >= 0; hAgo -= (int)labelEveryHours) {
    const float frac = ((float)windowHours - (float)hAgo) / (float)windowHours;
    const int x = x0 + (int)(frac * (float)plotW + 0.5f);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(padT);
    client.print(F("\" x2=\""));
    client.print(x);
    client.print(F("\" y2=\""));
    client.print(padT + plotH);
    client.println(F("\"/>"));

    int labelHour = nowTm.tm_hour - hAgo;
    while (labelHour < 0) labelHour += 24;
    while (labelHour >= 24) labelHour -= 24;

    bool isPm = (labelHour >= 12);
    int hour12 = labelHour % 12;
    if (hour12 == 0) hour12 = 12;

    client.print(F("<text text-anchor=\"middle\" x=\""));
    client.print(x);
    client.print(F("\" y=\""));
    client.print(H - 6);
    client.print(F("\">"));
    client.print(hour12);
    client.print(isPm ? "PM" : "AM");
    client.println(F("</text>"));
  }

  // pulses
  for (int i = 0; i < eventCount; i++) {
    uint32_t evMin2026 = 0;
    if (!eventDataGetMinute(i, evMin2026)) continue;
    if (evMin2026 > nowMin2026) continue;

    const uint32_t ageMin = nowMin2026 - evMin2026;
    if (ageMin > windowMinutes) continue;

    const float frac = 1.0f - ((float)ageMin / (float)windowMinutes);
    const int x = x0 + (int)(frac * (float)plotW + 0.5f);
    const int x2 = (x + pulseWidthPx < x1) ? (x + pulseWidthPx) : x1;

    // vertical up
    client.print(F("<line x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(yLow);
    client.print(F("\" x2=\""));
    client.print(x);
    client.print(F("\" y2=\""));
    client.print(yHigh);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));

    // top
    client.print(F("<line x1=\""));
    client.print(x);
    client.print(F("\" y1=\""));
    client.print(yHigh);
    client.print(F("\" x2=\""));
    client.print(x2);
    client.print(F("\" y2=\""));
    client.print(yHigh);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));

    // down
    client.print(F("<line x1=\""));
    client.print(x2);
    client.print(F("\" y1=\""));
    client.print(yHigh);
    client.print(F("\" x2=\""));
    client.print(x2);
    client.print(F("\" y2=\""));
    client.print(yLow);
    client.println(F("\" stroke=\"#2F6F73\" stroke-width=\"1\"/>"));
  }

  client.println(F("</svg></div>"));
}