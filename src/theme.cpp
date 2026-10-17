#include "theme.h"
#include "face_static.h"

const Theme THEMES[] = {
  { "Синий",    0,  90, 210,    0,  8,  45,   50, 100, 180,  120, 160, 220 },
  { "Чёрный",  40,  40,  55,    5,  5,  12,   90,  90, 110,  110, 110, 130 },
  { "Гранат", 120,  20,  30,   30,  0,   5,  160,  50,  60,  200, 120, 120 },
  { "Зелёный", 20, 100,  40,    0, 20,  10,   60, 150,  90,  120, 200, 140 },
};
const int THEME_COUNT = sizeof(THEMES) / sizeof(THEMES[0]);

void applyTheme(int idx) {
  g_currentTheme = ((idx % THEME_COUNT) + THEME_COUNT) % THEME_COUNT;
  const Theme& th = THEMES[g_currentTheme];

  g_gradCR = th.gradCR; g_gradCG = th.gradCG; g_gradCB = th.gradCB;
  g_gradER = th.gradER; g_gradEG = th.gradEG; g_gradEB = th.gradEB;

  C_RING     = tft.color565(th.ringR, th.ringG, th.ringB);
  C_TICK_MIN = tft.color565(th.tickR, th.tickG, th.tickB);

  // Цвет «дна» под VLW-текстом — совпадает с градиентом в этой точке
  C_STATUS_BG = gradientColorAt(CX, STATUS_Y);
  C_INFO_BG   = gradientColorAt(CX, FX_Y);

  bgSpr.fillSprite(TFT_BLACK);
  drawGradientDithered(bgSpr);
  drawTicks(bgSpr);
  drawRing(bgSpr);
  drawNumbers(bgSpr);
  drawTitle(bgSpr);
  drawDateFrame(bgSpr);

  Serial.printf("Theme applied: %s\n", th.name);
}