#include "charts.h"

#include "global.h"
#include "pumpData.h"
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
// Gallons / Day SVG chart
// -----------------------------------------------------------------------------
void renderGallonsPerDayChart(WiFiClient &client) {
  const int n = Pump.Daily365.dailyValidCount();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Gallons / Day (stored daily totals)</div>"));

  if (n <= 0) {
    client.println(F("<div class=\"small\">No daily totals saved yet.</div></div>"));
    return;
  }

  int maxG = -1;
  int minG = -1;

  for (int i = 0; i < n; i++) {
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;
    if (e.gallonsPerDay < 0  || e.gallonsPerDay > 10000) continue;

    if (i == 0) {
      maxG = e.gallonsPerDay;
      minG = e.gallonsPerDay;
    } else {
      if (e.gallonsPerDay > maxG) maxG = e.gallonsPerDay;
      if (e.gallonsPerDay < minG) minG = e.gallonsPerDay;
    }
  }

  // indicate if no valid values
  if (maxG < 0) {
    client.println(F("<div class=\"small\">No daily totals saved yet.</div></div>"));
    return;
  }

  uint32_t yMax = (uint32_t)(maxG * 1.1f + 50.0f);
  uint32_t yMin = (uint32_t)(minG * 0.9f);

  if (yMax < 100U) yMax = 100U;
  if (yMax <= yMin) yMax = yMin + 100U;

  const uint32_t range = yMax - yMin;

  uint32_t step;
  if (range < 900U)       step = 100U;
  else if (range < 2000U) step = 200U;
  else if (range < 2800U) step = 300U;
  else                    step = 500U;

  yMin = (yMin / step) * step;
  yMax = ((yMax + step - 1U) / step) * step;
  if (yMax <= yMin) yMax = yMin + step;

  const int W = 520, H = 180;
  const int padL = 38, padR = 12, padT = 16, padB = 28;
  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  const float denomY = (float)(yMax - yMin);

  for (uint32_t g = yMin; g <= yMax; g += step) {
    float t = ((float)g - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(padL);
    client.print(F("\" y1=\""));
    client.print(y);
    client.print(F("\" x2=\""));
    client.print(padL + plotW);
    client.print(F("\" y2=\""));
    client.print(y);
    client.println(F("\"/>"));

    client.print(F("<text text-anchor=\"end\" x=\"34\" y=\""));
    client.print(y + 4);
    client.print(F("\">"));
    client.print(g);
    client.println(F("</text>"));
  }

  client.print(F("<polyline class=\"line\" points=\""));

  for (int i = n - 1; i >= 0; i--) {
    const int idxChron = (n - 1) - i;
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;

    const int x = padL + (n == 1 ? plotW / 2 : (plotW * idxChron) / (n - 1));

    float t = ((float)e.gallonsPerDay - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(x); client.print(','); client.print(y); client.print(' ');
  }

  client.println(F("\"/>"));

  for (int i = n - 1; i >= 0; i--) {
    const int idxChron = (n - 1) - i;
    PumpDailyRecord e;
    if (!Pump.Daily365.getDailyRecordAgo(i, e)) continue;

    const int x = padL + (n == 1 ? plotW / 2 : (plotW * idxChron) / (n - 1));

    float t = ((float)e.gallonsPerDay - (float)yMin) / denomY;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<circle class=\"pt\" cx=\""));
    client.print(x);
    client.print(F("\" cy=\""));
    client.print(y);
    client.println(F("\" r=\"3\"/>"));
  }

  char oldestDate[16];
  char newestDate[16];
  getDayKeyForOffset(n, oldestDate, sizeof(oldestDate));  // last day stored
  getDayKeyForOffset(1, newestDate, sizeof(newestDate));  // yesterday

  client.print(F("<text x=\""));
  client.print(padL);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(oldestDate);
  client.println(F("</text>"));

  client.print(F("<text x=\""));
  client.print(padL + plotW - 90);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(newestDate);
  client.println(F("</text>"));

  client.println(F("</svg></div>"));
}

// -----------------------------------------------------------------------------
// Current waveform SVG chart
// -----------------------------------------------------------------------------
void renderCurrentWaveChart(WiFiClient &client) {
  const int n = Pump.getLastCycleCurrentSamplesCount();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Last Cycle Current Waveform (A)</div>"));

  if (n <= 0) {
    client.println(F("<div class=\"small\">No last-cycle current samples yet.</div></div>"));
    return;
  }

  const float minA = Pump.getLastCycleCurrentMinAmps();
  const float maxA = Pump.getLastCycleCurrentMaxAmps();

  int yMinI = (int)(0.9f * minA);
  int yMaxI = (int)(1.1f * maxA + 0.5f);
  if (yMaxI <= yMinI) yMaxI = yMinI + 1;

  const float yMin = (float)yMinI;
  const float yMax = (float)yMaxI;

  const int W = 520;
  const int H = 180;
  const int padL = 44;
  const int padR = 12;
  const int padT = 16;
  const int padB = 28;

  const int plotW = W - padL - padR;
  const int plotH = H - padT - padB;

  client.print(F("<svg class=\"chart\" viewBox=\"0 0 "));
  client.print(W); client.print(' '); client.print(H);
  client.println(F("\" xmlns=\"http://www.w3.org/2000/svg\">"));

  const float denom = (yMax - yMin);
  for (int a = yMinI; a <= yMaxI; a++) {
    float t = ((float)a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<line class=\"grid\" x1=\""));
    client.print(padL);
    client.print(F("\" y1=\""));
    client.print(y);
    client.print(F("\" x2=\""));
    client.print(padL + plotW);
    client.print(F("\" y2=\""));
    client.print(y);
    client.println(F("\"/>"));

    if ((a % 3) == 0) {
      client.print(F("<text text-anchor=\"end\" x=\""));
      client.print(padL - 6);
      client.print(F("\" y=\""));
      client.print(y + 4);
      client.print(F("\">"));
      client.print(a);
      client.println(F("</text>"));
    }
  }

  client.print(F("<polyline class=\"line\" points=\""));
  for (int i = 0; i < n; i++) {
    const float a = Pump.getLastCycleCurrentSampleAmps(i);
    const int x = padL + (n == 1 ? plotW / 2 : (plotW * i) / (n - 1));

    float t = (a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(x);
    client.print(',');
    client.print(y);
    client.print(' ');
  }
  client.println(F("\"/>"));

  for (int i = 0; i < n; i++) {
    const float a = Pump.getLastCycleCurrentSampleAmps(i);
    const int x = padL + (n == 1 ? plotW / 2 : (plotW * i) / (n - 1));

    float t = (a - yMin) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const int y = padT + plotH - (int)(t * (float)plotH);

    client.print(F("<circle class=\"pt\" cx=\""));
    client.print(x);
    client.print(F("\" cy=\""));
    client.print(y);
    client.println(F("\" r=\"2\"/>"));
  }

  client.print(F("<text x=\""));
  client.print(padL);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.println(F("\">0</text>"));

  client.print(F("<text x=\""));
  client.print(padL + plotW - 24);
  client.print(F("\" y=\""));
  client.print(padT + plotH + 20);
  client.print(F("\">"));
  client.print(n - 1);
  client.println(F("</text>"));

  client.println(F("</svg></div>"));
}

// -----------------------------------------------------------------------------
// Pump event 24h pulse chart
// -----------------------------------------------------------------------------
void renderPumpEvent24hChart(WiFiClient &client) {
  eventDataPurgeNow();

  client.println(F("<div class=\"chart-wrap\">"
                   "<div class=\"chart-title\">Pump Events - Last 24 Hours</div>"));

  const int eventCount = eventDataCount();
  if (eventCount <= 0) {
    client.println(F("<div class=\"small\">No pump events.</div></div>"));
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