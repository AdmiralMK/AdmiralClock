#include "net.h"
#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

constexpr const char *TZ_INFO = "UTC-5";
constexpr const char *NTP_SERVER = "pool.ntp.org";

void startWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
    Serial.println("WiFi started (non-blocking)");
}

void configNtp()
{
    configTzTime(TZ_INFO, NTP_SERVER);
}

bool fetchWeather()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += OWM_CITY;
    url += "&appid=";
    url += SECRET_OWM_API_KEY;
    url += "&units=metric&lang=ru";

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200)
    {
        Serial.printf("Weather HTTP %d\n", code);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
        Serial.print("Weather JSON error: ");
        Serial.println(err.c_str());
        return false;
    }

    g_tempC = doc["main"]["temp"] | 0.0f;
    const char *desc = doc["weather"][0]["description"] | "";
    strncpy(g_weatherDesc, desc, sizeof(g_weatherDesc) - 1);
    g_weatherDesc[sizeof(g_weatherDesc) - 1] = '\0';
    g_sunriseUnix = (time_t)(doc["sys"]["sunrise"] | 0L);
    g_sunsetUnix = (time_t)(doc["sys"]["sunset"] | 0L);
    g_sunValid = (g_sunriseUnix > 0 && g_sunsetUnix > 0);

    Serial.printf("Weather: %.1f°C, %s\n", g_tempC, g_weatherDesc);
    return true;
}

bool fetchFxRates()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;
    String url = "https://v6.exchangerate-api.com/v6/";
    url += SECRET_FX_API_KEY;
    url += "/latest/";
    url += FX_BASE;

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200)
    {
        Serial.printf("FX HTTP %d\n", code);
        http.end();
        return false;
    }
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
        Serial.print("FX JSON error: ");
        Serial.println(err.c_str());
        return false;
    }
    if (String(doc["result"] | "") != "success")
    {
        Serial.println("FX: non-success");
        return false;
    }

    float usd_per_rub = doc["conversion_rates"][FX_TARGET1] | 0.0f;
    float eur_per_rub = doc["conversion_rates"][FX_TARGET2] | 0.0f;
    float usdNew = (usd_per_rub > 0.0f) ? (1.0f / usd_per_rub) : 0.0f;
    float eurNew = (eur_per_rub > 0.0f) ? (1.0f / eur_per_rub) : 0.0f;

    // Сохраняем предыдущие значения для сравнения на следующем обновлении
    if (g_fxValid)
    {
        g_usdRubPrev = g_usdRub;
        g_eurRubPrev = g_eurRub;
        g_hasUsdPrev = (g_usdRub > 0.0f);
        g_hasEurPrev = (g_eurRub > 0.0f);
    }

    g_usdRub = usdNew;
    g_eurRub = eurNew;

    Serial.printf("FX: %s=%.2f  %s=%.2f\n", FX_TARGET1, g_usdRub, FX_TARGET2, g_eurRub);
    return true;
}