#pragma once
#include "globals.h"

// Собирает кадр (фон + инфо + луна + стрелки) и выводит на экран
void renderFrame(float aH, float aM, float aS, const struct tm* ti);

// Обработка кнопки BOOT (смена темы, дебаунс)
void handleButton();
void renderLevelFullscreen();