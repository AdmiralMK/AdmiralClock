#include "info_lines.h"

// ─────────────────────────────────────────────
// Обрезка UTF-8-строки до maxChars символов
// ─────────────────────────────────────────────
static void truncateUtf8(const char* src, char* dst, size_t dstSize,
                         int maxChars, bool ellipsis) {
  if (dstSize == 0) return;
  size_t out = 0;
  size_t i = 0;
  int count = 0;
  const size_t ellLen = ellipsis ? 3 : 0;   // 3 байта UTF-8 для "…"

  while (src[i] != '\0' && count < maxChars) {
    unsigned char c = (unsigned char)src[i];
    int len;
    if      ((c & 0x80) == 0x00) len = 1;
    else if ((c & 0xE0) == 0xC0) len = 2;
    else if ((c & 0xF0) == 0xE0) len = 3;
    else if ((c & 0xF8) == 0xF0) len = 4;
    else { i++; continue; }

    if (out + len + ellLen >= dstSize) break;
    for (int k = 0; k < len; k++) dst[out++] = src[i++];
    count++;
  }

  if (ellipsis && src[i] != '\0') {
    // U+2026 = "…" в UTF-8: 0xE2 0x80 0xA6 — один глиф на экране
    dst[out++] = (char)0xE2;
    dst[out++] = (char)0x80;
    dst[out++] = (char)0xA6;
  }
  dst[out] = '\0';
}

void drawDateText(TFT_eSprite &s, int day) {
  if (day < 0) return;
  s.loadFont(FONT_DATE);
  s.setTextDatum(MC_DATUM);
  s.setTextColor(TFT_BLACK, TFT_WHITE);
  s.drawString(String(day), DATE_X + DATE_W/2, DATE_Y + DATE_H/2);
  s.unloadFont();
}

void drawStatusText(TFT_eSprite &s, const struct tm* ti) {
  char buf[48];
  const char* msg = nullptr;

  if (phase == PHASE_RUN || phase == PHASE_SPIN) {
    if (SHOW_DATE_IN_STATUS && ti) {
      const char* wd = (ti->tm_wday >= 0 && ti->tm_wday < 7)
                       ? WEEKDAYS_RU[ti->tm_wday] : "";
      snprintf(buf, sizeof(buf), "%02d-%02d-%04d, %s",
               ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900, wd);
      msg = buf;
    } else return;
  } else {
    switch (phase) {
      case PHASE_WIFI:     msg = "Подключение к Wi-Fi";    break;
      case PHASE_SYNC:     msg = "Синхронизация времени";  break;
      case PHASE_NTP_FAIL: msg = "Ошибка синхронизации";   break;
      default: return;
    }
  }

  s.loadFont(FONT_STATUS);
  s.setTextDatum(BC_DATUM);
  if (INFO_SHADOW_ENABLED) {
    s.setTextColor(C_SHADOW, C_STATUS_BG);
    s.drawString(msg, INFO_X + INFO_SHADOW_DX, STATUS_Y + INFO_SHADOW_DY);
  }
  s.setTextColor(C_STATUS, C_STATUS_BG);
  s.drawString(msg, INFO_X, STATUS_Y);
  s.unloadFont();
}

void drawWeatherLine(TFT_eSprite &s) {
  if (!g_weatherValid) return;

  char full[96];
  snprintf(full, sizeof(full), "%.0f°C %s", g_tempC, g_weatherDesc);

  char buf[96];
  truncateUtf8(full, buf, sizeof(buf),
               WEATHER_LINE_MAX_CHARS, WEATHER_LINE_ELLIPSIS);

  s.loadFont(FONT_INFO);
  s.setTextDatum(BC_DATUM);
  if (INFO_SHADOW_ENABLED) {
    s.setTextColor(C_SHADOW, C_INFO_BG);
    s.drawString(buf, INFO_X + INFO_SHADOW_DX, WEATHER_Y + INFO_SHADOW_DY);
  }
  s.setTextColor(C_INFO, C_INFO_BG);
  s.drawString(buf, INFO_X, WEATHER_Y);
  s.unloadFont();
}

void drawFxLine(TFT_eSprite &s) {
  if (!g_fxValid) return;
  if (g_usdRub <= 0.0f && g_eurRub <= 0.0f) return;

  // ─── Стрелки изменения курса ───
  const char* usdArrow = "";
  const char* eurArrow = "";

  if (g_hasUsdPrev) {
    float d = g_usdRub - g_usdRubPrev;
    if (fabsf(d) >= FX_ARROW_MIN_DELTA) usdArrow = (d > 0) ? "↑" : "↓";
  }
  if (g_hasEurPrev) {
    float d = g_eurRub - g_eurRubPrev;
    if (fabsf(d) >= FX_ARROW_MIN_DELTA) eurArrow = (d > 0) ? "↑" : "↓";
  }

  char buf[48];
  snprintf(buf, sizeof(buf), "$%.2f%s €%.2f%s",
           g_usdRub, usdArrow, g_eurRub, eurArrow);

  s.loadFont(FONT_INFO);
  s.setTextDatum(BC_DATUM);
  if (INFO_SHADOW_ENABLED) {
    s.setTextColor(C_SHADOW, C_INFO_BG);
    s.drawString(buf, INFO_X + INFO_SHADOW_DX, FX_Y + INFO_SHADOW_DY);
  }
  s.setTextColor(C_INFO, C_INFO_BG);
  s.drawString(buf, INFO_X, FX_Y);
  s.unloadFont();
}

void drawSunLine(TFT_eSprite &s) {
  if (!g_sunValid) return;
  time_t now = time(nullptr);
  time_t eventUnix;
  const char* label;
  if (now < g_sunriseUnix) {
    eventUnix = g_sunriseUnix;  label = "Всх";
  } else if (now < g_sunsetUnix) {
    eventUnix = g_sunsetUnix;   label = "Зхд";
  } else {
    eventUnix = g_sunriseUnix + 86400;
    label     = "Всх";
  }
  struct tm et;
  localtime_r(&eventUnix, &et);
  char buf[32];
  snprintf(buf, sizeof(buf), "%s %02d:%02d", label, et.tm_hour, et.tm_min);
  s.loadFont(FONT_INFO);
  s.setTextDatum(BC_DATUM);
  if (INFO_SHADOW_ENABLED) {
    s.setTextColor(C_SHADOW, C_INFO_BG);
    s.drawString(buf, INFO_X + INFO_SHADOW_DX, SUN_Y + INFO_SHADOW_DY);
  }
  s.setTextColor(C_INFO, C_INFO_BG);
  s.drawString(buf, INFO_X, SUN_Y);
  s.unloadFont();
}