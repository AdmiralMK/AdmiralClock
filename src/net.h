#pragma once
#include "globals.h"

// Инициализация Wi-Fi (без блокировки)
void startWiFi();

// Настраивает NTP после подключения
void configNtp();

// Запросы данных
bool fetchWeather();
bool fetchFxRates();