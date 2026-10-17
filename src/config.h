#pragma once
#pragma GCC diagnostic ignored "-Wcpp"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <config_font.h>

constexpr bool GRADIENT_DITHER_ENABLED = true;

// ─── ИДЕНТИФИКАЦИЯ ───
#define CLOCK_TITLE   "Admiral"

// ─── ПИНЫ / ДИСПЛЕЙ ───
// Пины дисплея задаются в platformio.ini через build_flags.

// ─── КНОПКА BOOT ───
#define BTN_BOOT_PIN          0
constexpr unsigned long BTN_DEBOUNCE_MS = 30;

// ─── КНОПКА BOOT ───
constexpr unsigned long DOUBLE_CLICK_WINDOW_MS = 350;

// ─── ЭКРАН / СПРАЙТ ───
constexpr int  CX       = 120;
constexpr int  CY       = 120;
constexpr int  SCR_W    = 240;
constexpr int  SCR_H    = 240;
constexpr int  SPR_BYTES = SCR_W * SCR_H;



// ─── ИКОНКА ЛУНЫ ───
constexpr bool     MOON_ENABLED          = true;
constexpr int      MOON_DIST_FROM_CENTER = 57;
constexpr int      MOON_ICON_SIZE        = 20;
constexpr int      MOON_SHADOW_DX        = 2;
constexpr int      MOON_SHADOW_DY        = 2;
constexpr unsigned long MOON_PULSE_MS    = 3000;

// ─── MPU / I2C ───
#define IMU_SDA_PIN       21
#define IMU_SCL_PIN       22
#define IMU_I2C_FREQ      400000
constexpr bool IMU_CALIBRATE_ON_BOOT = false;   // true → калибровка при старте
                                                // (нужно держать часы горизонтально)

// ─── ИНДИКАТОР УРОВНЯ (справа от луны) ───
constexpr bool LEVEL_ENABLED          = true;
constexpr int  LEVEL_DIST_FROM_CENTER = 28;   // расстояние от центра (по X)
constexpr int  LEVEL_ICON_SIZE        = 20;   // диаметр иконки
constexpr int  LEVEL_SHADOW_DX        = 2;
constexpr int  LEVEL_SHADOW_DY        = 2;
constexpr int  LEVEL_BUBBLE_RADIUS    = 3;
constexpr int  LEVEL_PIXELS_PER_G     = 12;   // чувствительность (±1g → ±12 px)

// ─── МИШЕНЬ УРОВНЯ (два круга + перекрестье) ───
constexpr bool LEVEL_TARGET_ENABLED     = true;
constexpr int  LEVEL_TARGET_OUTER_OFF   = 2;   // отступ внешнего круга от канта (px)
constexpr int  LEVEL_TARGET_INNER_R     = 4;   // радиус внутреннего круга (px)
constexpr bool LEVEL_CROSSHAIR_ENABLED  = true;

// Оси: подстраивается под ориентацию датчика. Если пузырёк движется
// в неправильную сторону — переключите соответствующий флаг.
constexpr bool LEVEL_SWAP_XY   = false;
constexpr bool LEVEL_INVERT_X  = false;
constexpr bool LEVEL_INVERT_Y  = false;

// ─── ЗАГОЛОВОК ───
constexpr int  TITLE_X = CX;
constexpr int  TITLE_Y = 82;
constexpr bool TITLE_SHADOW_ENABLED = true;
constexpr int  TITLE_SHADOW_DX      = 1;
constexpr int  TITLE_SHADOW_DY      = 1;

// ─── СТАРТОВОЕ ВРЕМЯ ───
constexpr int DEFAULT_HOUR = 12;
constexpr int DEFAULT_MIN  = 0;

// ─── ПРОКРУТКА СТРЕЛОК ───
constexpr unsigned long SPIN_DURATION_MS = 1800;
constexpr int SPIN_HOUR_TURNS = 2;
constexpr int SPIN_MIN_TURNS  = 1;
constexpr int SPIN_SEC_TURNS  = 3;
constexpr unsigned long WIFI_TIMEOUT_MS = 15000;
constexpr unsigned long NTP_TIMEOUT_MS  = 30000;

// ─── API ───
constexpr const char* OWM_CITY    = "Ufa,RU";
constexpr const char* FX_BASE     = "RUB";
constexpr const char* FX_TARGET1  = "USD";
constexpr const char* FX_TARGET2  = "EUR";

constexpr unsigned long WEATHER_UPDATE_MS = 15UL * 60UL * 1000UL;
constexpr unsigned long FX_UPDATE_MS      = 60UL * 60UL * 1000UL;

// ─── ИНДИКАТОР ИЗМЕНЕНИЯ КУРСА ───
// Минимальное изменение в рублях, чтобы показать стрелку.
// 0.005 = полкопейки → стрелка появляется при любом видимом изменении
//          на двух знаках после запятой.
// 0.05 → стрелка только при изменении ≥5 копеек (меньше дёрганья).
// 0.5 → стрелка только при изменении ≥50 копеек (для «спокойного» вида).
constexpr float FX_ARROW_MIN_DELTA = 0.005f;

// ─── ГЕОМЕТРИЯ ───
constexpr int SCREEN_MARGIN       = 8;
constexpr int TICK_HOUR_LENGTH    = 13;
constexpr int TICK_MIN_LENGTH     = 5;
constexpr int NUMBERS_RADIUS_BASE = 82;
constexpr int NUMBERS_RADIUS_FIX  = 2;

constexpr int R_OUTER      = (SCR_W / 2) - SCREEN_MARGIN;
constexpr int R_HOUR_INNER = R_OUTER - TICK_HOUR_LENGTH;
constexpr int R_MIN_INNER  = R_OUTER - TICK_MIN_LENGTH;
constexpr int NUMBERS_R    = NUMBERS_RADIUS_BASE + NUMBERS_RADIUS_FIX;

// ─── ВНЕШНЕЕ КОЛЬЦО ───
constexpr bool RING_ENABLED   = true;
constexpr int  RING_OFFSET    = 2;
constexpr int  RING_THICKNESS = 2;

// ─── ТЕНЬ ПОД ЦИФРАМИ ───
constexpr bool NUMBER_SHADOW_ENABLED = true;
constexpr int  NUMBER_SHADOW_DX      = 2;
constexpr int  NUMBER_SHADOW_DY      = 2;

// ─── БЛИК СТУПИЦЫ ───
constexpr bool HUB_HIGHLIGHT_ENABLED = true;
constexpr int  HUB_HL_DX = -1;
constexpr int  HUB_HL_DY = -1;
constexpr int  HUB_HL_R  = 1;

// ─── ДАТА ───
constexpr int DATE_X = 163;
constexpr int DATE_Y = 114;
constexpr int DATE_W = 22;
constexpr int DATE_H = 14;
constexpr int DATE_FRAME_OUTER = 3;
constexpr int DATE_FRAME_INNER = 2;

// ─── ИНФО-ОБЛАСТЬ (4 строки) ───
constexpr int INFO_X    = CX;
constexpr int INFO_STEP = 12;
constexpr int STATUS_Y  = 156;
constexpr int WEATHER_Y = STATUS_Y  + INFO_STEP;
constexpr int FX_Y      = WEATHER_Y + INFO_STEP;
constexpr int SUN_Y     = FX_Y      + INFO_STEP;

constexpr bool INFO_SHADOW_ENABLED = true;
constexpr int  INFO_SHADOW_DX      = 1;
constexpr int  INFO_SHADOW_DY      = 1;
constexpr bool SHOW_DATE_IN_STATUS = true;

// ─── ОБРЕЗКА СТРОКИ ПОГОДЫ ───
constexpr int  WEATHER_LINE_MAX_CHARS = 19;   // макс. символов (UTF-8, не байт)
constexpr bool WEATHER_LINE_ELLIPSIS  = true; // добавлять "..." если обрезали

// ─── ДНИ НЕДЕЛИ ───
extern const char* WEEKDAYS_RU[7];

// ─────────────────────────────────────────────
// ТЕМЫ
// ─────────────────────────────────────────────
struct Theme {
  const char* name;
  uint8_t gradCR, gradCG, gradCB;
  uint8_t gradER, gradEG, gradEB;
  uint8_t ringR, ringG, ringB;
  uint8_t tickR, tickG, tickB;
};

extern const Theme THEMES[];
extern const int   THEME_COUNT;

// ─────────────────────────────────────────────
// ПОЛНОЭКРАННЫЙ УРОВЕНЬ
// ─────────────────────────────────────────────
constexpr int LEVEL_FULL_R              = 100;  // радиус большого круга
constexpr int LEVEL_FULL_BUBBLE_R       = 14;   // радиус пузырька
constexpr int LEVEL_FULL_PIXELS_PER_G   = 55;   // чувствительность
constexpr int LEVEL_FULL_OUTER_OFF      = 8;    // отступ внешнего круга мишени
constexpr int LEVEL_FULL_MID_R          = 50;   // средний круг мишени
constexpr int LEVEL_FULL_INNER_R        = 20;   // внутренний круг мишени


// ─── МОНИТОРИНГ RSS НОВОСТЕЙ ───
constexpr const char* NEWS_RSS_URL = "https://gkchs.bashkortostan.ru/presscenter/news/rss/";
constexpr unsigned long NEWS_UPDATE_MS = 2UL * 60UL * 1000UL;   // 2 минут
constexpr int NEWS_ITEMS_TO_CHECK = 2;                           // проверять 2 последние новости

// ─── БАННЕР ПРЕДУПРЕЖДЕНИЯ ───
constexpr bool ALERT_ENABLED = true;
constexpr int ALERT_BANNER_X = 40;
constexpr int ALERT_BANNER_Y = 90;
constexpr int ALERT_BANNER_W = 160;
constexpr int ALERT_BANNER_H = 16;
constexpr int ALERT_SHADOW_DX = 2;
constexpr int ALERT_SHADOW_DY = 2;
constexpr unsigned long ALERT_BLINK_PERIOD_MS = 1000;            // период пульсации (мс)
constexpr unsigned long CANCEL_BLINK_DURATION_MS = 5UL * 60UL * 1000UL; // 5 минут мигания
constexpr unsigned long CANCEL_TOTAL_DURATION_MS = 15UL * 60UL * 1000UL; // 15 минут всего
constexpr int ALERT_CORNER_RADIUS = 3;   // радиус скругления баннера, px (0 = прямые углы)

