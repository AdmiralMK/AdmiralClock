#include "render.h"
#include "theme.h"
#include "face_dynamic.h"
#include "info_lines.h"
#include "storage.h"
#include "imu.h"
#include "news_monitor.h"

// forward declaration — реализация ниже в этом же файле
static void drawAlertBanner(TFT_eSprite &s);

void renderFrame(float aH, float aM, float aS, const struct tm* ti) {
  memcpy(workSpr.getPointer(), bgSpr.getPointer(), SPR_BYTES);

  int day = ti ? ti->tm_mday : -1;
  drawDateText(workSpr, day);
  drawStatusText(workSpr, ti);
  drawWeatherLine(workSpr);
  drawFxLine(workSpr);
  drawSunLine(workSpr);

  if (ti) {
    float phase = moonPhase(time(nullptr));
    float t = (float)(millis() % MOON_PULSE_MS) / (float)MOON_PULSE_MS;
    float pulse = 0.5f - 0.5f * cosf(t * 2.0f * PI);
    drawMoonIcon(workSpr, phase, pulse);
    drawLevelIcon(workSpr);
  }

  drawHand(workSpr, aH, 55, 6, 3, C_HAND_RED,  C_HAND_HILITE);
  drawHand(workSpr, aM, 80, 5, 2, C_HAND_RED,  C_HAND_HILITE);
  drawHand(workSpr, aS, 92, 3, 1, C_HAND_DARK, C_HAND_HILITE);

  drawHub(workSpr);
  drawAlertBanner(workSpr);
  workSpr.pushSprite(0, 0);
}

void handleButton()
{
  static int lastStable = HIGH;
  static unsigned long firstPressMs = 0;
  static bool pendingClick = false;
  unsigned long now = millis();

  int raw = digitalRead(BTN_BOOT_PIN);

  // Реагируем только на смену состояния (фронт нажатия / отпускания)
  if (raw != lastStable)
  {
    // Простейший дебаунс: если прошло меньше BTN_DEBOUNCE_MS — игнорируем
    static unsigned long lastEdgeMs = 0;
    if (now - lastEdgeMs < BTN_DEBOUNCE_MS)
    {
      // слишком быстро для реального клика — пропускаем
    }
    else
    {
      lastEdgeMs = now;
      lastStable = raw;

      if (raw == LOW)
      { // именно нажатие
        if (pendingClick && (now - firstPressMs) < DOUBLE_CLICK_WINDOW_MS)
        {
          // Второе нажатие в окне → двойной клик
          pendingClick = false;
          g_viewMode = (g_viewMode == VIEW_CLOCK) ? VIEW_LEVEL : VIEW_CLOCK;
          Serial.printf("View mode: %s\n",
                        g_viewMode == VIEW_CLOCK ? "CLOCK" : "LEVEL");
        }
        else
        {
          // Первое нажатие — запоминаем, ждём второго
          pendingClick = true;
          firstPressMs = now;
        }
      }
    }
  }

  // Одиночный клик — если второе нажатие не пришло за DOUBLE_CLICK_WINDOW_MS
  if (pendingClick && (now - firstPressMs) > DOUBLE_CLICK_WINDOW_MS)
  {
    pendingClick = false;
    applyTheme(g_currentTheme + 1);
    storageSaveTheme(g_currentTheme);
    Serial.printf("Theme: %d\n", g_currentTheme);
  }
}

void renderLevelFullscreen()
{
  drawLevelFullscreen(workSpr);
}

// ─────────────────────────────────────────────
// БАННЕР ПРЕДУПРЕЖДЕНИЯ
// ─────────────────────────────────────────────

// ─────────────────────────────────────────────
// Величина горизонтального отступа для строки
// скруглённого прямоугольника (row = 0..h-1).
// ─────────────────────────────────────────────
static inline int roundRectInset(int row, int h, int r) {
  if (r <= 0) return 0;
  if (row < r) {
    int dy = r - row;
    float dx = sqrtf((float)(r * r - dy * dy));
    return r - (int)dx;
  } else if (row >= h - r) {
    int dy = row - (h - r - 1);
    float dx = sqrtf((float)(r * r - dy * dy));
    return r - (int)dx;
  }
  return 0;
}

// ─────────────────────────────────────────────
// БАННЕР ПРЕДУПРЕЖДЕНИЯ
// ─────────────────────────────────────────────
static void drawAlertBanner(TFT_eSprite &s) {
  if (!ALERT_ENABLED) return;
  if (g_alertState == ALERT_NONE) return;

  unsigned long elapsed = millis() - g_alertStartMs;

  // Для отмены — проверяем таймауты
  bool isCancel = (g_alertState == CANCEL_DRONE ||
                   g_alertState == CANCEL_ROCKET);
  if (isCancel && elapsed > CANCEL_TOTAL_DURATION_MS) {
    g_alertState = ALERT_NONE;
    return;
  }

  // ─── Текст и цвета градиента ───
  const char* text = nullptr;
  uint8_t rTop, gTop, bTop;
  uint8_t rMid, gMid, bMid;

  switch (g_alertState) {
    case ALERT_DRONE:
      text = "Беспилотная опасность!";
      rTop = 150; gTop = 0;  bTop = 0;
      rMid = 255; gMid = 50; bMid = 50;
      break;
    case ALERT_ROCKET:
      text = "Ракетная опасность!";
      rTop = 150; gTop = 0;  bTop = 0;
      rMid = 255; gMid = 50; bMid = 50;
      break;
    case CANCEL_DRONE:
      text = "Отмена опасности БПЛА";
      rTop = 0;   gTop = 120; bTop = 0;
      rMid = 50;  gMid = 255; bMid = 50;
      break;
    case CANCEL_ROCKET:
      text = "Отмена ракетной опасности";
      rTop = 0;   gTop = 120; bTop = 0;
      rMid = 50;  gMid = 255; bMid = 50;
      break;
    default:
      return;
  }

  // ─── Мигание ───
  bool doBlink = true;
  if (isCancel && elapsed > CANCEL_BLINK_DURATION_MS) doBlink = false;

  float brightness = 1.0f;
  if (doBlink) {
    float t = (float)(millis() % ALERT_BLINK_PERIOD_MS) / ALERT_BLINK_PERIOD_MS;
    brightness = 0.5f + 0.5f * sinf(t * 2.0f * PI);   // 0..1..0
  }

  // ─── Радиус скругления (с защитой от R > H/2) ───
  int R = ALERT_CORNER_RADIUS;
  if (R > ALERT_BANNER_H / 2) R = ALERT_BANNER_H / 2;
  if (R < 0) R = 0;

  // ─── Тень (тоже скруглённая) ───
  for (int i = 0; i < ALERT_BANNER_H; i++) {
    int inset = roundRectInset(i, ALERT_BANNER_H, R);
    s.drawFastHLine(ALERT_BANNER_X + ALERT_SHADOW_DX + inset,
                    ALERT_BANNER_Y + ALERT_SHADOW_DY + i,
                    ALERT_BANNER_W - 2 * inset, TFT_BLACK);
  }

  // ─── Градиентный фон со скруглёнными углами ───
  for (int i = 0; i < ALERT_BANNER_H; i++) {
    int inset = roundRectInset(i, ALERT_BANNER_H, R);

    float t = (float)i / (ALERT_BANNER_H - 1);
    uint8_t r, g, b;
    if (t < 0.5f) {
      float k = t * 2.0f;
      r = (uint8_t)((1 - k) * rTop + k * rMid);
      g = (uint8_t)((1 - k) * gTop + k * gMid);
      b = (uint8_t)((1 - k) * bTop + k * bMid);
    } else {
      float k = (t - 0.5f) * 2.0f;
      r = (uint8_t)((1 - k) * rMid + k * rTop);
      g = (uint8_t)((1 - k) * gMid + k * gTop);
      b = (uint8_t)((1 - k) * bMid + k * bTop);
    }
    r = (uint8_t)(r * brightness);
    g = (uint8_t)(g * brightness);
    b = (uint8_t)(b * brightness);

    s.drawFastHLine(ALERT_BANNER_X + inset, ALERT_BANNER_Y + i,
                    ALERT_BANNER_W - 2 * inset, tft.color565(r, g, b));
  }

  // ─── Текст с тенью ───
  s.loadFont(FONT_STATUS);
  s.setTextDatum(MC_DATUM);

  // тень текста
  s.setTextColor(TFT_BLACK, TFT_BLACK);
  s.drawString(text,
               ALERT_BANNER_X + ALERT_BANNER_W / 2 + 1,
               ALERT_BANNER_Y + ALERT_BANNER_H / 2 + 1);

  // сам текст
  s.setTextColor(TFT_WHITE, TFT_BLACK);
  s.drawString(text,
               ALERT_BANNER_X + ALERT_BANNER_W / 2,
               ALERT_BANNER_Y + ALERT_BANNER_H / 2);

  s.unloadFont();
}