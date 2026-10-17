#include "imu.h"
#include "face_dynamic.h"

void drawHand(TFT_eSprite &s, float angleDeg, int length,
              int baseW, int tipW, uint16_t color, uint16_t hilite) {
  float a = (angleDeg - 90.0f) * DEG_TO_RAD;
  float ca = cosf(a), sa = sinf(a);
  int tipX = CX + (int)(ca * length), tipY = CY + (int)(sa * length);
  float perp = a + M_PI_2;
  float cP = cosf(perp), sP = sinf(perp);
  int bx1 = CX + (int)(cP * baseW), by1 = CY + (int)(sP * baseW);
  int bx2 = CX - (int)(cP * baseW), by2 = CY - (int)(sP * baseW);
  int tx1 = tipX + (int)(cP * tipW), ty1 = tipY + (int)(sP * tipW);
  int tx2 = tipX - (int)(cP * tipW), ty2 = tipY - (int)(sP * tipW);
  s.fillTriangle(tipX+2, tipY+2, bx1+2, by1+2, bx2+2, by2+2, C_SHADOW);
  s.fillTriangle(tipX+2, tipY+2, tx1+2, ty1+2, tx2+2, ty2+2, C_SHADOW);
  s.fillTriangle(tipX, tipY, bx1, by1, bx2, by2, color);
  s.fillTriangle(tipX, tipY, tx1, ty1, tx2, ty2, color);
  s.drawLine(bx1, by1, tipX, tipY, hilite);
}

void drawHub(TFT_eSprite &s) {
  s.fillCircle(CX, CY, 5, C_HAND_DARK);
  s.fillCircle(CX, CY, 3, C_HAND_RED);
  if (HUB_HIGHLIGHT_ENABLED)
    s.fillCircle(CX + HUB_HL_DX, CY + HUB_HL_DY, HUB_HL_R, C_HAND_HILITE);
}

void drawMoonIcon(TFT_eSprite &s, float phase, float pulse) {
  if (!MOON_ENABLED) return;
  int cx = CX - MOON_DIST_FROM_CENTER;
  int cy = CY;
  int r  = MOON_ICON_SIZE / 2;

  s.fillCircle(cx + MOON_SHADOW_DX, cy + MOON_SHADOW_DY, r, C_MOON_SHADOW);

  if (pulse > 0.15f) {
    uint8_t gv = (uint8_t)((pulse - 0.15f) / 0.85f * 180.0f);
    s.drawCircle(cx, cy, r + 1, tft.color565(gv/3, gv/3, gv/2));
  }
  if (pulse > 0.55f) {
    uint8_t gv = (uint8_t)((pulse - 0.55f) / 0.45f * 140.0f);
    s.drawCircle(cx, cy, r + 2, tft.color565(gv/4, gv/4, gv/3));
  }

  s.fillCircle(cx, cy, r, C_MOON_BASE);

  float phaseAngle = 2.0f * PI * phase;
  float cosPhase   = cosf(phaseAngle);
  bool  waxing     = (phase < 0.5f);
  float x_t_eq = waxing ? ((float)r * cosPhase) : (-(float)r * cosPhase);

  for (int dy = -r; dy <= r; dy++) {
    int xFull = (int)sqrtf((float)(r*r - dy*dy));
    if (xFull <= 0) continue;
    int x_t = (int)(x_t_eq * (float)xFull / (float)r);
    if (waxing) {
      int xStart = -xFull, xEnd = x_t;
      if (xEnd > xStart)
        s.drawFastHLine(cx + xStart, cy + dy, xEnd - xStart, C_MOON_DARK);
    } else {
      int xStart = x_t + 1, xEnd = xFull + 1;
      if (xEnd > xStart)
        s.drawFastHLine(cx + xStart, cy + dy, xEnd - xStart, C_MOON_DARK);
    }
  }

  struct { int dx, dy; bool big; } craters[] = {
    { -3, -2, false }, {  1, -4, false }, {  3,  1, true  },
    { -1,  3, false }, {  4, -3, false },
  };
  for (auto &c : craters) {
    int px = cx + c.dx, py = cy + c.dy;
    if (c.big) s.fillCircle(px, py, 1, C_MOON_CRATER);
    else       s.drawPixel(px, py, C_MOON_CRATER);
  }

  int hlx = waxing ? (cx + 3) : (cx - 3);
  int hly = cy - 3;
  int twinkleR = (int)(pulse * 2.5f);

  if (twinkleR >= 1) {
    s.drawFastHLine(hlx - twinkleR, hly, twinkleR * 2 + 1, C_MOON_HIGHLIGHT);
    s.drawFastVLine(hlx, hly - twinkleR, twinkleR * 2 + 1, C_MOON_HIGHLIGHT);
  }
  s.fillCircle(hlx, hly, twinkleR >= 1 ? 1 : 0, C_MOON_HIGHLIGHT);
  s.drawPixel(hlx, hly, tft.color565(255, 255, 255));

  if (pulse > 0.7f) {
    uint16_t sparkGlow = tft.color565(255, 230, 140);
    s.drawPixel(hlx - twinkleR - 1, hly, sparkGlow);
    s.drawPixel(hlx + twinkleR + 1, hly, sparkGlow);
    s.drawPixel(hlx, hly - twinkleR - 1, sparkGlow);
    s.drawPixel(hlx, hly + twinkleR + 1, sparkGlow);
  }
}

// ─────────────────────────────────────────────
// ИНДИКАТОР УРОВНЯ (пузырёк в воде)
// ─────────────────────────────────────────────
void drawLevelIcon(TFT_eSprite &s) {
  if (!LEVEL_ENABLED) return;
  if (!g_imuValid) return;

  int cx = CX - LEVEL_DIST_FROM_CENTER;
  int cy = CY;
  int r  = LEVEL_ICON_SIZE / 2;

  // 1. Тень
  s.fillCircle(cx + LEVEL_SHADOW_DX, cy + LEVEL_SHADOW_DY, r, C_LEVEL_SHADOW);

  // 2. Вода (белый диск)
  s.fillCircle(cx, cy, r, C_LEVEL_WATER);

  // 3. Двойной кант (границы иконки)
  s.drawCircle(cx, cy, r,     C_LEVEL_BORDER);
  s.drawCircle(cx, cy, r - 1, C_LEVEL_BORDER);

  // 4. ── МИШЕНЬ ──
  if (LEVEL_TARGET_ENABLED) {
    // Внешний круг мишени — на 2 px меньше канта
    int rOuter = r - LEVEL_TARGET_OUTER_OFF;
    if (rOuter > 2) s.drawCircle(cx, cy, rOuter, C_LEVEL_TARGET);

    // Внутренний круг мишени
    if (LEVEL_TARGET_INNER_R > 0)
      s.drawCircle(cx, cy, LEVEL_TARGET_INNER_R, C_LEVEL_TARGET);

    // Перекрестье — короткие лучики по 4 направлениям,
    // идут от внутреннего круга до внешнего
    if (LEVEL_CROSSHAIR_ENABLED) {
      int armIn  = LEVEL_TARGET_INNER_R + 1;
      int armOut = rOuter - 1;
      if (armOut > armIn) {
        s.drawFastHLine(cx - armOut, cy, armOut - armIn + 1, C_LEVEL_TARGET);
        s.drawFastHLine(cx + armIn,  cy, armOut - armIn + 1, C_LEVEL_TARGET);
        s.drawFastVLine(cx, cy - armOut, armOut - armIn + 1, C_LEVEL_TARGET);
        s.drawFastVLine(cx, cy + armIn,  armOut - armIn + 1, C_LEVEL_TARGET);
      }
    }
  }

  // 5. Куда сместить пузырёк (по акселерометру)
  float ax = g_accelX;
  float ay = g_accelY;
  if (LEVEL_SWAP_XY) { float t = ax; ax = ay; ay = t; }
  if (LEVEL_INVERT_X) ax = -ax;
  if (LEVEL_INVERT_Y) ay = -ay;

  int maxOff = r - LEVEL_BUBBLE_RADIUS - 2;
  int bx = cx + constrain((int)(ax * LEVEL_PIXELS_PER_G), -maxOff, maxOff);
  int by = cy + constrain((int)(ay * LEVEL_PIXELS_PER_G), -maxOff, maxOff);

  // 6. Пузырёк
  s.fillCircle(bx, by, LEVEL_BUBBLE_RADIUS, C_LEVEL_BUBBLE);
  s.drawCircle(bx, by, LEVEL_BUBBLE_RADIUS, C_LEVEL_BUBBLE_DARK);

  // 7. Блик на пузырьке
  s.drawPixel(bx - 1, by - 1, TFT_WHITE);
}

// ─────────────────────────────────────────────
// ПОЛНОЭКРАННЫЙ УРОВЕНЬ
// ─────────────────────────────────────────────
void drawLevelFullscreen(TFT_eSprite &s) {
  // Фон
  s.fillSprite(C_INFO_BG);

  // Если датчик не найден / упал
  if (!g_imuValid) {
    s.loadFont(FONT_INFO);
    s.setTextDatum(MC_DATUM);
    s.setTextColor(C_STATUS, C_INFO_BG);
    s.drawString("IMU не найден", CX, CY);
    s.unloadFont();
    s.pushSprite(0, 0);
    return;
  }

  const int R = LEVEL_FULL_R;
  const int bubR = LEVEL_FULL_BUBBLE_R;

  // 1. Тень
  s.fillCircle(CX + 2, CY + 2, R, C_LEVEL_SHADOW);

  // 2. Вода
  s.fillCircle(CX, CY, R, C_LEVEL_WATER);

  // 3. Двойной кант
  s.drawCircle(CX, CY, R,     C_LEVEL_BORDER);
  s.drawCircle(CX, CY, R - 1, C_LEVEL_BORDER);

  // 4. Мишень: три круга
  int rOuter = R - LEVEL_FULL_OUTER_OFF;
  s.drawCircle(CX, CY, rOuter,                C_LEVEL_TARGET);
  s.drawCircle(CX, CY, LEVEL_FULL_MID_R,      C_LEVEL_TARGET);
  s.drawCircle(CX, CY, LEVEL_FULL_INNER_R,    C_LEVEL_TARGET);

  // 5. Перекрестье — от внутреннего круга до внешнего
  int armIn  = LEVEL_FULL_INNER_R + 1;
  int armOut = rOuter - 1;
  if (armOut > armIn) {
    int len = armOut - armIn + 1;
    s.drawFastHLine(CX - armOut, CY, len, C_LEVEL_TARGET);
    s.drawFastHLine(CX + armIn,  CY, len, C_LEVEL_TARGET);
    s.drawFastVLine(CX, CY - armOut, len, C_LEVEL_TARGET);
    s.drawFastVLine(CX, CY + armIn,  len, C_LEVEL_TARGET);
  }

  // 6. Пузырёк — положение по акселерометру
  float ax = g_accelX;
  float ay = g_accelY;
  if (LEVEL_SWAP_XY)  { float t = ax; ax = ay; ay = t; }
  if (LEVEL_INVERT_X) ax = -ax;
  if (LEVEL_INVERT_Y) ay = -ay;

  int maxOff = R - BR - 8;
  int bx = CX + constrain((int)(ax * LEVEL_FULL_PIXELS_PER_G), -maxOff, maxOff);
  int by = CY + constrain((int)(ay * LEVEL_FULL_PIXELS_PER_G), -maxOff, maxOff);

  s.fillCircle(bx, by, BR, C_LEVEL_BUBBLE);
  s.drawCircle(bx, by, BR,     C_LEVEL_BUBBLE_DARK);
  s.drawCircle(bx, by, BR - 1, C_LEVEL_BUBBLE_DARK);

  // Блик на пузырьке
  s.drawPixel(bx - 4, by - 4, TFT_WHITE);
  s.drawPixel(bx - 3, by - 4, TFT_WHITE);
  s.drawPixel(bx - 4, by - 3, TFT_WHITE);
  s.drawPixel(bx - 5, by - 3, TFT_WHITE);

  // 7. Углы наклона в градусах (без зависимости от ориентации датчика)
  float mag = sqrtf(ax*ax + ay*ay + g_accelZ*g_accelZ);
  if (mag < 0.1f) mag = 0.1f;
  float tiltX = atan2f(ax, sqrtf(ay*ay + g_accelZ*g_accelZ)) * 180.0f / PI;
  float tiltY = atan2f(ay, sqrtf(ax*ax + g_accelZ*g_accelZ)) * 180.0f / PI;

  char buf[40];
  snprintf(buf, sizeof(buf), "X: %+.1f°  Y: %+.1f°", tiltX, tiltY);

  s.loadFont(FONT_INFO);
  s.setTextDatum(MC_DATUM);
  s.setTextColor(C_STATUS, C_INFO_BG);
  s.drawString(buf, CX, 232);
  s.unloadFont();

  s.pushSprite(0, 0);
}