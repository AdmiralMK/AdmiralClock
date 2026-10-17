#include "globals.h"
#include "secrets.h"

// ─── ОБЪЕКТЫ ───
TFT_eSPI tft;
TFT_eSprite bgSpr(&tft);
TFT_eSprite workSpr(&tft);

// ─── ФАЗЫ ───
Phase phase = PHASE_WIFI;
unsigned long phaseStart = 0;
unsigned long spinStart = 0;
float spinStartH = 0;
float spinStartM = 0;
float spinStartS = 0;

// ─── ТЕМА ───
int g_currentTheme = 0;
ViewMode g_viewMode = VIEW_CLOCK;

// ─── ГРАДИЕНТ ───
uint8_t g_gradCR = 0, g_gradCG = 90, g_gradCB = 210;
uint8_t g_gradER = 0, g_gradEG = 8, g_gradEB = 45;

// ─── ЦВЕТА ───
uint16_t C_HAND_RED, C_HAND_DARK, C_HAND_HILITE, C_SHADOW;
uint16_t C_TICK_MIN, C_DATE_BORDER, C_TITLE, C_STATUS, C_RING;
uint16_t C_STATUS_BG;
uint16_t C_DATE_INSET_DARK, C_DATE_INSET_LIGHT;
uint16_t C_INFO, C_INFO_BG;
uint16_t C_MOON_BASE, C_MOON_DARK, C_MOON_CRATER, C_MOON_HIGHLIGHT, C_MOON_SHADOW;
uint16_t C_LEVEL_WATER, C_LEVEL_BUBBLE, C_LEVEL_BUBBLE_DARK;
uint16_t C_LEVEL_BORDER, C_LEVEL_SHADOW;
uint16_t C_LEVEL_TARGET;

// ─── ДАННЫЕ ───
float g_tempC = 0.0f;
char g_weatherDesc[48] = {0};
float g_usdRub = 0.0f;
float g_eurRub = 0.0f;
bool g_weatherValid = false;
bool g_fxValid = false;
time_t g_sunriseUnix = 0;
time_t g_sunsetUnix = 0;
bool g_sunValid = false;
unsigned long g_lastWeatherMs = 0;
unsigned long g_lastFxMs = 0;
float g_usdRubPrev = 0.0f;
float g_eurRubPrev = 0.0f;
bool  g_hasUsdPrev = false;
bool  g_hasEurPrev = false;

// ─── ДНИ НЕДЕЛИ ───
const char *WEEKDAYS_RU[7] = {
    "Воскресенье", "Понедельник", "Вторник",
    "Среда", "Четверг", "Пятница", "Суббота"};

// ─── РЕАЛИЗАЦИЯ УТИЛИТ ───
void initColors()
{
  C_HAND_RED = tft.color565(220, 30, 30);
  C_HAND_DARK = tft.color565(140, 10, 10);
  C_HAND_HILITE = tft.color565(255, 140, 140);
  C_SHADOW = tft.color565(5, 5, 25);
  C_DATE_BORDER = tft.color565(200, 205, 220);
  C_TITLE = tft.color565(255, 200, 80);
  C_STATUS = tft.color565(220, 230, 255);
  C_DATE_INSET_DARK = tft.color565(130, 145, 180);
  C_DATE_INSET_LIGHT = tft.color565(245, 248, 255);
  C_INFO = tft.color565(230, 240, 255);
  C_MOON_BASE = tft.color565(255, 240, 230);
  C_MOON_DARK = tft.color565(60, 70, 110);
  C_MOON_CRATER = tft.color565(220, 200, 140);
  C_MOON_HIGHLIGHT = tft.color565(255, 255, 230);
  C_MOON_SHADOW = tft.color565(40, 50, 150);
  // C_RING, C_TICK_MIN, C_STATUS_BG, C_INFO_BG — задаются в applyTheme()
  C_LEVEL_WATER = tft.color565(245, 250, 255);     // почти белая «вода»
  C_LEVEL_BUBBLE = tft.color565(150, 240, 150);    // светло-зелёный
  C_LEVEL_BUBBLE_DARK = tft.color565(60, 170, 60); // кант пузырька
  C_LEVEL_BORDER = tft.color565(120, 150, 190);    // серо-синий кант
  C_LEVEL_SHADOW = tft.color565(0, 5, 30);         // тень
  C_LEVEL_TARGET = tft.color565(180, 195, 210);
}

float angleHour(int h, int m, int s, float sub)
{
  return (h % 12) * 30.0f + m * 0.5f + (s + sub) * (0.5f / 60.0f);
}
float angleMin(int m, int s, float sub)
{
  return m * 6.0f + (s + sub) * 0.1f;
}
float angleSec(int s, float sub)
{
  return (s + sub) * 6.0f;
}
bool timeIsValid()
{
  return time(nullptr) > 1700000000UL;
}

uint16_t gradientColorAt(int x, int y)
{
  int dx = x - CX, dy = y - CY;
  int r2 = dx * dx + dy * dy;
  float t = (r2 > 120 * 120) ? 1.0f : sqrtf((float)r2) / 120.0f;
  int R = (int)(g_gradCR * (1.0f - t) + g_gradER * t);
  int G = (int)(g_gradCG * (1.0f - t) + g_gradEG * t);
  int B = (int)(g_gradCB * (1.0f - t) + g_gradEB * t);
  return tft.color565(R, G, B);
}

float moonPhase(time_t t)
{
  const time_t NEW_MOON_REF = 947182440;
  const float SYNODIC = 29.530588853f;
  double days = (double)(t - NEW_MOON_REF) / 86400.0;
  double phase = fmod(days / SYNODIC, 1.0);
  if (phase < 0)
    phase += 1.0;
  return (float)phase;
}