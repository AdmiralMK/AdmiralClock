#pragma once
#include "config.h"
#include <WiFi.h>          // ← добавить
#include <time.h>          // ← на всякий случай
#include <sys/time.h>      // ← для gettimeofday
#include <math.h>          // ← для cosf/sinf/sqrtf
#include "news_monitor.h"

// ─── РЕЖИМ ОТОБРАЖЕНИЯ ───
enum ViewMode { VIEW_CLOCK, VIEW_LEVEL };
extern ViewMode g_viewMode;

// ─── ОБЪЕКТЫ ───
extern TFT_eSPI    tft;
extern TFT_eSprite bgSpr;
extern TFT_eSprite workSpr;

// ─── ФАЗЫ ───
enum Phase { PHASE_WIFI, PHASE_SYNC, PHASE_NTP_FAIL, PHASE_SPIN, PHASE_RUN };
extern Phase         phase;
extern unsigned long phaseStart;
extern unsigned long spinStart;
extern float         spinStartH;
extern float         spinStartM;
extern float         spinStartS;

// ─── ТЕМА ───
extern int g_currentTheme;

// ─── ГРАДИЕНТ ───
extern uint8_t g_gradCR, g_gradCG, g_gradCB;
extern uint8_t g_gradER, g_gradEG, g_gradEB;

// ─── ЦВЕТА ───
extern uint16_t C_HAND_RED, C_HAND_DARK, C_HAND_HILITE, C_SHADOW;
extern uint16_t C_TICK_MIN, C_DATE_BORDER, C_TITLE, C_STATUS, C_RING;
extern uint16_t C_STATUS_BG;
extern uint16_t C_DATE_INSET_DARK, C_DATE_INSET_LIGHT;
extern uint16_t C_INFO, C_INFO_BG;
extern uint16_t C_MOON_BASE, C_MOON_DARK, C_MOON_CRATER, C_MOON_HIGHLIGHT, C_MOON_SHADOW;
extern uint16_t C_LEVEL_WATER, C_LEVEL_BUBBLE, C_LEVEL_BUBBLE_DARK;
extern uint16_t C_LEVEL_BORDER, C_LEVEL_SHADOW;
extern uint16_t C_LEVEL_TARGET;

// ─── ДАННЫЕ ИЗ ИНТЕРНЕТА ───
extern float   g_tempC;
extern char    g_weatherDesc[48];
extern float   g_usdRub;
extern float   g_eurRub;
extern bool    g_weatherValid;
extern bool    g_fxValid;
extern time_t  g_sunriseUnix;
extern time_t  g_sunsetUnix;
extern bool    g_sunValid;
extern unsigned long g_lastWeatherMs;
extern unsigned long g_lastFxMs;
extern float g_usdRubPrev;
extern float g_eurRubPrev;
extern bool  g_hasUsdPrev;
extern bool  g_hasEurPrev;

// ─── УТИЛИТЫ ───
void initColors();
float angleHour(int h, int m, int s, float sub);
float angleMin(int m, int s, float sub);
float angleSec(int s, float sub);
bool  timeIsValid();
uint16_t gradientColorAt(int x, int y);
float moonPhase(time_t t);