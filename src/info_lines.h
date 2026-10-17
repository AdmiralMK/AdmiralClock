#pragma once
#include "globals.h"

void drawDateText(TFT_eSprite &s, int day);
void drawStatusText(TFT_eSprite &s, const struct tm* ti);
void drawWeatherLine(TFT_eSprite &s);
void drawFxLine(TFT_eSprite &s);
void drawSunLine(TFT_eSprite &s);