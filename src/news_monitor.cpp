#include "news_monitor.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <WiFiClientSecure.h>

AlertState g_alertState = ALERT_NONE;
unsigned long g_alertStartMs = 0;

// ─────────────────────────────────────────────
// Ручной режим для тестирования баннера
// ─────────────────────────────────────────────
// false — обычная работа: newsMonitorUpdate() раз в NEWS_UPDATE_MS
//         опрашивает RSS и обновляет g_alertState по реальным новостям.
// true  — тестовый режим: RSS игнорируется, состояние баннера задаётся
//         вручную командами Serial Monitor (1–4, 5–6, 7).
//         Снимается командами '0' (сброс) или 'n' (принудительный RSS-опрос).
// После перезагрузки ESP32 всегда сбрасывается в false.
bool g_manualOverride = false;

// ─────────────────────────────────────────────
// Приведение UTF-8 к нижнему регистру (A-Z, А-Я, Ё)
// ─────────────────────────────────────────────
static void toLowerCaseUtf8(String &s)
{
    for (int i = 0; i < (int)s.length(); i++)
    {
        uint8_t c = (uint8_t)s[i];
        if (c >= 0x41 && c <= 0x5A)
        {
            s[i] = c + 0x20;
        }
        else if (c == 0xD0 && i + 1 < (int)s.length())
        {
            uint8_t n = (uint8_t)s[i + 1];
            if (n >= 0x90 && n <= 0x9F)
            {
                s[i + 1] = n + 0x20;
            }
            else if (n >= 0xA0 && n <= 0xAF)
            {
                s[i] = 0xD1;
                s[i + 1] = n - 0x20 + 0x80;
            }
            else if (n == 0x81)
            {
                s[i] = 0xD1;
                s[i + 1] = 0x91;
            }
            i++;
        }
        else if (c == 0xD1 && i + 1 < (int)s.length())
        {
            i++;
        }
    }
}

// ─────────────────────────────────────────────
// Извлечение содержимого тега (с пропуском атрибутов)
// ─────────────────────────────────────────────
static String extractTag(const String &src, const String &tag)
{
    String open = "<" + tag;
    int s = src.indexOf(open);
    if (s < 0)
        return "";
    int gt = src.indexOf('>', s);
    if (gt < 0)
        return "";
    s = gt + 1;
    String close = "</" + tag + ">";
    int e = src.indexOf(close, s);
    if (e < 0)
        return "";
    return src.substring(s, e);
}

static String stripCdata(const String &s)
{
    int start = s.indexOf("<![CDATA[");
    if (start < 0)
        return s;
    start += 9;
    int end = s.indexOf("]]>", start);
    if (end < 0)
        return s;
    return s.substring(start, end);
}

// ─────────────────────────────────────────────
// Русские месяцы (в UTF-8, 3 буквы = 6 байт)
// ─────────────────────────────────────────────
static int parseRussianMonth(const char *p)
{
    static const char *months[12] = {
        "янв", "фев", "мар", "апр", "май", "июн",
        "июл", "авг", "сен", "окт", "ноя", "дек"};
    for (int i = 0; i < 12; i++)
    {
        if (strncmp(p, months[i], 6) == 0)
            return i;
    }
    return -1;
}

// ─────────────────────────────────────────────
// Парсинг pubDate: "Пт, 16 окт 2026 11:45:00 +0500"
// Возвращает unix timestamp или 0 при ошибке.
// TZ ESP32 = UTC+5 (МСК+2), совпадает с +0500 из ленты.
// ─────────────────────────────────────────────
static time_t parsePubDate(const String &s)
{
    int comma = s.indexOf(',');
    if (comma < 0)
        return 0;
    int pos = comma + 1;
    while (pos < (int)s.length() && s[pos] == ' ')
        pos++;

    // День
    int d = 0;
    while (pos < (int)s.length() && isdigit((unsigned char)s[pos]))
    {
        d = d * 10 + (s[pos] - '0');
        pos++;
    }
    if (d < 1 || d > 31)
        return 0;
    while (pos < (int)s.length() && s[pos] == ' ')
        pos++;

    // Месяц
    int m = parseRussianMonth(s.c_str() + pos);
    if (m < 0)
        return 0;
    pos += 6; // 3 русские буквы × 2 байта
    while (pos < (int)s.length() && s[pos] == ' ')
        pos++;

    // Год
    int y = 0;
    while (pos < (int)s.length() && isdigit((unsigned char)s[pos]))
    {
        y = y * 10 + (s[pos] - '0');
        pos++;
    }
    if (y < 2000 || y > 2100)
        return 0;
    while (pos < (int)s.length() && s[pos] == ' ')
        pos++;

    // HH:MM:SS
    if (pos + 8 > (int)s.length())
        return 0;
    int hh = (s[pos] - '0') * 10 + (s[pos + 1] - '0');
    pos += 2;
    if (s[pos] == ':')
        pos++;
    int mm = (s[pos] - '0') * 10 + (s[pos + 1] - '0');
    pos += 2;
    if (s[pos] == ':')
        pos++;
    int ss = (s[pos] - '0') * 10 + (s[pos + 1] - '0');

    struct tm tm = {};
    tm.tm_year = y - 1900;
    tm.tm_mon = m;
    tm.tm_mday = d;
    tm.tm_hour = hh;
    tm.tm_min = mm;
    tm.tm_sec = ss;
    tm.tm_isdst = -1;
    time_t t = mktime(&tm);
    return (t > 0) ? t : 0;
}

void newsMonitorBegin()
{
    g_alertState = ALERT_NONE;
    g_alertStartMs = 0;
}

// ─────────────────────────────────────────────
// Один опрос RSS, разбор 2 последних новостей
// ─────────────────────────────────────────────
void newsMonitorUpdate()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(8000);
    http.setUserAgent("Mozilla/5.0 (OlyaClock)");
    http.begin(client, NEWS_RSS_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = http.GET();
    if (code != 200)
    {
        Serial.printf("News RSS HTTP %d\n", code);
        http.end();
        return;
    }
    String payload = http.getString();
    http.end();

    int start = 0;
    int count = 0;
    while (count < NEWS_ITEMS_TO_CHECK)
    {
        int itemStart = payload.indexOf("<item>", start);
        if (itemStart < 0)
            break;
        int itemEnd = payload.indexOf("</item>", itemStart);
        if (itemEnd < 0)
            break;

        String item = payload.substring(itemStart + 6, itemEnd);
        String full = stripCdata(extractTag(item, "title")) + " " +
                      stripCdata(extractTag(item, "description"));
        toLowerCaseUtf8(full);

        time_t pub = parsePubDate(stripCdata(extractTag(item, "pubDate")));
        time_t now = time(nullptr);
        unsigned long ageMs = (pub > 0 && now > pub)
                                  ? (unsigned long)(now - pub) * 1000UL
                                  : 0;

        // ─── ОТМЕНЫ (проверяются первыми — специфичнее) ───
        bool isCancelDrone =
            full.indexOf("отмена беспилотной") >= 0 ||
            full.indexOf("отбой беспилотной") >= 0;
        bool isCancelRocket =
            full.indexOf("отмена ракетной") >= 0 ||
            full.indexOf("отбой ракетной") >= 0;

        if (isCancelDrone || isCancelRocket)
        {
            if (ageMs > CANCEL_TOTAL_DURATION_MS)
            {
                Serial.printf("News: cancel too old (%lu min), skip\n",
                              ageMs / 60000UL);
                return; // отмену нашли, она старая — выходим; alert старше её не нужен
            }
            g_alertState = isCancelDrone ? CANCEL_DRONE : CANCEL_ROCKET;
            g_alertStartMs = millis() - ageMs;
            Serial.printf("News: CANCEL_%s (age=%lu s)\n",
                          isCancelDrone ? "DRONE" : "ROCKET",
                          ageMs / 1000UL);
            return;
        }

        // ─── ТРЕВОГИ ───
        if (full.indexOf("беспилотная опасность") >= 0)
        {
            g_alertState = ALERT_DRONE;
            g_alertStartMs = millis();
            Serial.println("News: ALERT_DRONE");
            return;
        }
        if (full.indexOf("ракетная опасность") >= 0)
        {
            g_alertState = ALERT_ROCKET;
            g_alertStartMs = millis();
            Serial.println("News: ALERT_ROCKET");
            return;
        }

        start = itemEnd + 7;
        count++;
    }

    Serial.println("News: no alert keywords in last 2 items");
}

// ─────────────────────────────────────────────
// Обработка Serial-команд для тестирования баннера
// ─────────────────────────────────────────────
void newsMonitorHandleSerial()
{
    if (!Serial.available())
        return;
    int c = Serial.read();
    if (c == '\r' || c == '\n' || c < 0)
        return;

    unsigned long now = millis();

    switch (c)
    {
    case '1':
        g_alertState = ALERT_DRONE;
        g_alertStartMs = now;
        g_manualOverride = true;
        Serial.println("[TEST] ALERT_DRONE (красный, мигает)");
        break;

    case '2':
        g_alertState = ALERT_ROCKET;
        g_alertStartMs = now;
        g_manualOverride = true;
        Serial.println("[TEST] ALERT_ROCKET (красный, мигает)");
        break;

    case '3':
        g_alertState = CANCEL_DRONE;
        g_alertStartMs = now;
        g_manualOverride = true;
        Serial.println("[TEST] CANCEL_DRONE (зелёный, мигает 15 мин)");
        break;

    case '4':
        g_alertState = CANCEL_ROCKET;
        g_alertStartMs = now;
        g_manualOverride = true;
        Serial.println("[TEST] CANCEL_ROCKET (зелёный, мигает 15 мин)");
        break;

    case '5':
        g_alertState = CANCEL_DRONE;
        g_alertStartMs = now - CANCEL_BLINK_DURATION_MS - 1000;
        g_manualOverride = true;
        Serial.println("[TEST] CANCEL_DRONE (зелёный, статика)");
        break;

    case '6':
        g_alertState = CANCEL_ROCKET;
        g_alertStartMs = now - CANCEL_BLINK_DURATION_MS - 1000;
        g_manualOverride = true;
        Serial.println("[TEST] CANCEL_ROCKET (зелёный, статика)");
        break;

    case '7':
        g_alertState = CANCEL_DRONE;
        g_alertStartMs = now - CANCEL_TOTAL_DURATION_MS + 60000UL; // исчезнет через 60 сек
        g_manualOverride = true;
        Serial.println("[TEST] CANCEL_DRONE (исчезнет через 60 сек)");
        break;

    case '0':
        g_alertState = ALERT_NONE;
        g_alertStartMs = 0;
        g_manualOverride = false;
        Serial.println("[TEST] сброс, override OFF");
        break;

    case 'n':
    case 'N':
        g_manualOverride = false;
        Serial.println("[TEST] принудительный RSS-опрос");
        newsMonitorUpdate();
        break;

    case '?':
        Serial.printf("[STATUS] state=%d override=%d elapsed=%lu s, heap=%u\n",
                      (int)g_alertState, (int)g_manualOverride,
                      (millis() - g_alertStartMs) / 1000,
                      ESP.getFreeHeap());
        break;

    default:
        Serial.println("[HELP]");
        Serial.println("  1 = Drone alert (красный, мигает)");
        Serial.println("  2 = Rocket alert (красный, мигает)");
        Serial.println("  3 = Cancel Drone (зелёный, мигает)");
        Serial.println("  4 = Cancel Rocket (зелёный, мигает)");
        Serial.println("  5 = Cancel Drone (зелёный, статика)");
        Serial.println("  6 = Cancel Rocket (зелёный, статика)");
        Serial.println("  7 = Cancel Drone (исчезнет через 60 сек)");
        Serial.println("  0 = сброс / выйти из теста");
        Serial.println("  n = принудительный RSS-опрос");
        Serial.println("  ? = статус");
        break;
    }
}
