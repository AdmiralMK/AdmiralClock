#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────
// Энергонезависимое хранилище (NVS)
// ─────────────────────────────────────────────
// Простое обёртка над Preferences для сохранения пользовательских
// настроек между перезагрузками.

void storageBegin();

// Тема
int  storageLoadTheme(int fallback);
void storageSaveTheme(int idx);