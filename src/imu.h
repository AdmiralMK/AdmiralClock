#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────
// IMU: MPU-6500 / MPU-9250 (совместимые регистры)
// ─────────────────────────────────────────────
extern bool    g_imuFound;    // устройство ответило на I2C
extern bool    g_imuValid;    // данные актуальны
extern uint8_t g_imuWhoAmI;   // значение регистра WHO_AM_I
extern float   g_accelX;      // в единицах g (после калибровки)
extern float   g_accelY;
extern float   g_accelZ;

void imuBegin();
void imuUpdate();
const char* imuName();