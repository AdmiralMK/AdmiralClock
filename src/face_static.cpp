#include "face_static.h"

// ─── ГРАДИЕНТ (полный RGB332: RRRGGGBB) ───
void drawGradientDithered(TFT_eSprite &s)
{

  if (!GRADIENT_DITHER_ENABLED)
  {
    // Простой градиент без дизеринга
    for (int r = 120; r >= 0; r--)
    {
      float t = (float)r / 120.0f;
      int R = (int)(g_gradCR * (1.0f - t) + g_gradER * t);
      int G = (int)(g_gradCG * (1.0f - t) + g_gradEG * t);
      int B = (int)(g_gradCB * (1.0f - t) + g_gradEB * t);
      s.fillCircle(CX, CY, r, tft.color565(R, G, B));
    }
    return;
  }

  uint8_t *buf = (uint8_t *)s.getPointer();
  if (!buf)
    return;
  static const uint8_t bayer[4][4] = {
      {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  for (int y = 0; y < SCR_H; y++)
  {
    int dy = y - CY, dy2 = dy * dy;
    uint8_t *row = buf + y * SCR_W;
    for (int x = 0; x < SCR_W; x++)
    {
      int dx = x - CX;
      int r2 = dx * dx + dy2;
      if (r2 > 120 * 120)
      {
        row[x] = 0;
        continue;
      }
      float t = sqrtf((float)r2) / 120.0f;
      float Rf = g_gradCR * (1.0f - t) + g_gradER * t;
      float Gf = g_gradCG * (1.0f - t) + g_gradEG * t;
      float Bf = g_gradCB * (1.0f - t) + g_gradEB * t;
      int bv = bayer[y & 3][x & 3];

      float rS = (Rf / 255.0f) * 7.0f;
      int rI = (int)rS;
      if ((rS - rI) * 16.0f > bv)
        rI++;
      if (rI < 0)
        rI = 0;
      if (rI > 7)
        rI = 7;

      float gS = (Gf / 255.0f) * 7.0f;
      int gI = (int)gS;
      if ((gS - gI) * 16.0f > bv)
        gI++;
      if (gI < 0)
        gI = 0;
      if (gI > 7)
        gI = 7;

      float bS = (Bf / 255.0f) * 3.0f;
      int bI = (int)bS;
      if ((bS - bI) * 16.0f > bv)
        bI++;
      if (bI < 0)
        bI = 0;
      if (bI > 3)
        bI = 3;

      row[x] = (rI << 5) | (gI << 2) | bI;
    }
  }
}

void drawTicks(TFT_eSprite &s)
{
  for (int m = 0; m < 60; m++)
  {
    float angle = (m * 6.0f - 90.0f) * DEG_TO_RAD;
    bool isHour = (m % 5 == 0);
    int rOuter = R_OUTER;
    int rInner = isHour ? R_HOUR_INNER : R_MIN_INNER;
    int x1 = CX + (int)(cosf(angle) * rInner);
    int y1 = CY + (int)(sinf(angle) * rInner);
    int x2 = CX + (int)(cosf(angle) * rOuter);
    int y2 = CY + (int)(sinf(angle) * rOuter);
    uint16_t col = isHour ? TFT_WHITE : C_TICK_MIN;
    s.drawLine(x1, y1, x2, y2, col);
    if (isHour)
    {
      int x1b = CX + (int)(cosf(angle) * (rInner - 2));
      int y1b = CY + (int)(sinf(angle) * (rInner - 2));
      s.drawLine(x1b, y1b, x2, y2, col);
    }
  }
}

void drawRing(TFT_eSprite &s)
{
  if (!RING_ENABLED)
    return;
  int r = R_OUTER + RING_OFFSET;
  s.drawCircle(CX, CY, r, C_RING);
  if (RING_THICKNESS >= 2)
    s.drawCircle(CX, CY, r + 1, C_RING);
}

void drawNumbers(TFT_eSprite &s)
{
  s.setTextDatum(MC_DATUM);
  s.loadFont(FONT_NUMBERS);
  for (int h = 1; h <= 12; h++)
  {
    float angle = (h * 30.0f - 90.0f) * DEG_TO_RAD;
    int x = CX + (int)(cosf(angle) * NUMBERS_R);
    int y = CY + (int)(sinf(angle) * NUMBERS_R);
    if (NUMBER_SHADOW_ENABLED)
    {
      int sx = x + NUMBER_SHADOW_DX, sy = y + NUMBER_SHADOW_DY;
      s.setTextColor(C_SHADOW, gradientColorAt(sx, sy));
      s.drawString(String(h), sx, sy);
    }
    s.setTextColor(TFT_WHITE, gradientColorAt(x, y));
    s.drawString(String(h), x, y);
  }
  s.unloadFont();
}

void drawTitle(TFT_eSprite &s)
{
  s.loadFont(FONT_TITLE);
  s.setTextDatum(MC_DATUM);
  if (TITLE_SHADOW_ENABLED)
  {
    int sx = TITLE_X + TITLE_SHADOW_DX, sy = TITLE_Y + TITLE_SHADOW_DY;
    s.setTextColor(C_SHADOW, gradientColorAt(sx, sy));
    s.drawString(CLOCK_TITLE, sx, sy);
  }
  s.setTextColor(C_TITLE, gradientColorAt(TITLE_X, TITLE_Y));
  s.drawString(CLOCK_TITLE, TITLE_X, TITLE_Y);
  s.unloadFont();
}

void drawDateFrame(TFT_eSprite &s)
{
  const int ox = DATE_FRAME_OUTER, ix = DATE_FRAME_INNER;
  s.drawRect(DATE_X - ox, DATE_Y - ox, DATE_W + ox * 2, DATE_H + ox * 2, C_DATE_BORDER);
  s.drawRect(DATE_X - ix, DATE_Y - ix, DATE_W + ix * 2, DATE_H + ix * 2, C_DATE_INSET_DARK);
  s.fillRect(DATE_X, DATE_Y, DATE_W, DATE_H, TFT_WHITE);
  s.drawLine(DATE_X, DATE_Y, DATE_X + DATE_W - 1, DATE_Y, C_DATE_INSET_DARK);
  s.drawLine(DATE_X, DATE_Y, DATE_X, DATE_Y + DATE_H - 1, C_DATE_INSET_DARK);
  s.drawLine(DATE_X, DATE_Y + DATE_H - 1, DATE_X + DATE_W - 1, DATE_Y + DATE_H - 1, C_DATE_INSET_LIGHT);
  s.drawLine(DATE_X + DATE_W - 1, DATE_Y, DATE_X + DATE_W - 1, DATE_Y + DATE_H - 1, C_DATE_INSET_LIGHT);
}