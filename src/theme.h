#pragma once
#include "globals.h"

// Применяет тему по индексу (с циклическим перебором),
// обновляет g_grad*, C_RING, C_TICK_MIN, C_STATUS_BG, C_INFO_BG
// и полностью перерисовывает bgSpr.
void applyTheme(int idx);