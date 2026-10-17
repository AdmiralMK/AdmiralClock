/*
 * OlyaClock v16 — модульная версия
 * Пины: RST=G4, CS=G19, DC=G16, SDA=G23, SCL=G18
 */

#pragma GCC diagnostic ignored "-Wcpp"

#include "globals.h"
#include "theme.h"
#include "net.h"
#include "render.h"
#include "storage.h"
#include "imu.h"
#include "news_monitor.h"


// ─── SETUP ───
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nOlyaClock starting...");

  pinMode(BTN_BOOT_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  initColors();

  bgSpr.setColorDepth(8);
  workSpr.setColorDepth(8);
  if (!bgSpr.createSprite(SCR_W, SCR_H) || !workSpr.createSprite(SCR_W, SCR_H))
  {
    Serial.println("FATAL: sprite allocation failed");
    while (true)
      delay(1000);
  }
  Serial.printf("Sprites OK, free heap: %u\n", ESP.getFreeHeap());

  imuBegin();
  newsMonitorBegin();

  // ─── Чтение сохранённой темы из NVS ───
  storageBegin();
  int savedTheme = storageLoadTheme(0); // 0 = синяя, если ничего не сохранено
  applyTheme(savedTheme);

  phase = PHASE_WIFI;
  phaseStart = millis();

  renderFrame(angleHour(DEFAULT_HOUR, DEFAULT_MIN, 0, 0),
              angleMin(DEFAULT_MIN, 0, 0), 0, nullptr);

  startWiFi();
}

// ─── LOOP ───
void loop()
{
  static unsigned long lastFrame = 0;
  unsigned long now = millis();
  if (now - lastFrame < 40)
    return;
  lastFrame = now;

  newsMonitorHandleSerial(); 
  handleButton();

  // ─── Фазовая машина ───
  switch (phase)
  {
  case PHASE_WIFI:
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi OK -> SYNC");
      phase = PHASE_SYNC;
      phaseStart = now;
      configNtp();
    }
    else if (now - phaseStart > WIFI_TIMEOUT_MS)
    {
      Serial.println("WiFi timeout, retry");
      WiFi.disconnect();
      startWiFi();
      phaseStart = now;
    }
    break;

  case PHASE_SYNC:
    if (timeIsValid())
    {
      Serial.println("NTP OK -> SPIN");
      spinStart = now;
      spinStartH = angleHour(DEFAULT_HOUR, DEFAULT_MIN, 0, 0);
      spinStartM = angleMin(DEFAULT_MIN, 0, 0);
      spinStartS = 0;
      phase = PHASE_SPIN;
    }
    else if (now - phaseStart > NTP_TIMEOUT_MS)
    {
      phase = PHASE_NTP_FAIL;
      phaseStart = now;
    }
    break;

  case PHASE_NTP_FAIL:
    if (timeIsValid())
    {
      spinStart = now;
      spinStartH = angleHour(DEFAULT_HOUR, DEFAULT_MIN, 0, 0);
      spinStartM = angleMin(DEFAULT_MIN, 0, 0);
      spinStartS = 0;
      phase = PHASE_SPIN;
    }
    else if (WiFi.status() != WL_CONNECTED)
    {
      phase = PHASE_WIFI;
      phaseStart = now;
      WiFi.reconnect();
    }
    break;

  case PHASE_SPIN:
    if (now - spinStart > SPIN_DURATION_MS)
      phase = PHASE_RUN;
    break;

  case PHASE_RUN:
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.reconnect();
      phase = PHASE_WIFI;
      phaseStart = now;
    }
    break;
  }

  // ─── Периодические обновления данных ───
  if (phase == PHASE_RUN)
  {
    // Погода/курсы — как было
    if (now - g_lastWeatherMs > WEATHER_UPDATE_MS || !g_weatherValid)
    {
      if (fetchWeather())
        g_weatherValid = true;
      g_lastWeatherMs = now;
    }
    if (now - g_lastFxMs > FX_UPDATE_MS || !g_fxValid)
    {
      if (fetchFxRates())
        g_fxValid = true;
      g_lastFxMs = now;
    }

    // ─── IMU — отдельно, 10 раз в секунду ───
    static unsigned long lastImuRead = 0;
    if (now - lastImuRead > 100)
    {
      imuUpdate();
      lastImuRead = now;
    }
    
    // ─── RSS новости ───
    static unsigned long lastNewsMs = 0;
    if (now - lastNewsMs > NEWS_UPDATE_MS)
    {
      newsMonitorUpdate();
      lastNewsMs = now;
    }
  }

  // ─── Если включён полноэкранный уровень — рисуем его и выходим ───
  if (g_viewMode == VIEW_LEVEL)
  {
    imuUpdate();
    renderLevelFullscreen();
    return;
  }

  // ─── Рендер ───
  if (phase == PHASE_RUN)
  {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm ti;
    localtime_r(&tv.tv_sec, &ti);
    float sub = tv.tv_usec / 1000000.0f;
    renderFrame(angleHour(ti.tm_hour, ti.tm_min, ti.tm_sec, sub),
                angleMin(ti.tm_min, ti.tm_sec, sub),
                angleSec(ti.tm_sec, sub), &ti);
    return;
  }

  if (phase == PHASE_SPIN)
  {
    float p = (now - spinStart) / (float)SPIN_DURATION_MS;
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm ti;
    localtime_r(&tv.tv_sec, &ti);
    float sub = tv.tv_usec / 1000000.0f;
    float tH = angleHour(ti.tm_hour, ti.tm_min, ti.tm_sec, sub);
    float tM = angleMin(ti.tm_min, ti.tm_sec, sub);
    float tS = angleSec(ti.tm_sec, sub);
    float ease = 1.0f - powf(1.0f - p, 3.0f);
    renderFrame(spinStartH + ease * ((tH - spinStartH) + 360.0f * SPIN_HOUR_TURNS),
                spinStartM + ease * ((tM - spinStartM) + 360.0f * SPIN_MIN_TURNS),
                spinStartS + ease * ((tS - spinStartS) + 360.0f * SPIN_SEC_TURNS),
                &ti);
    return;
  }

  renderFrame(angleHour(DEFAULT_HOUR, DEFAULT_MIN, 0, 0),
              angleMin(DEFAULT_MIN, 0, 0), 0, nullptr);
}