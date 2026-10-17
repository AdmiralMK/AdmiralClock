#pragma once
#include "globals.h"

void drawHand(TFT_eSprite &s, float angleDeg, int length,
              int baseW, int tipW, uint16_t color, uint16_t hilite);
void drawHub(TFT_eSprite &s);
void drawMoonIcon(TFT_eSprite &s, float phase, float pulse);
void drawLevelIcon(TFT_eSprite &s);
void drawLevelFullscreen(TFT_eSprite &s);