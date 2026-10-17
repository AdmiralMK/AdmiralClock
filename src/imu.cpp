#include "imu.h"
#include "config.h"
#include <Wire.h>

// ─── Состояние ───
bool    g_imuFound = false;
bool    g_imuValid = false;
uint8_t g_imuWhoAmI = 0;
float   g_accelX = 0.0f;
float   g_accelY = 0.0f;
float   g_accelZ = 0.0f;

static uint8_t s_addr = 0;
static int     s_failCount = 0;

// ─── Калибровочные смещения (в g) ───
static float s_offX = 0.0f;
static float s_offY = 0.0f;

// ─── I2C helpers ───
static bool i2cRead(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, (int)len) != (int)len) return false;
  for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

static bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

const char* imuName() {
  switch (g_imuWhoAmI) {
    case 0x70: return "MPU-6500";
    case 0x71: return "MPU-9250";
    case 0x73: return "MPU-9255";
    case 0x68: return "MPU-6050";
    case 0x19: return "MPU-6886";
    default:   return "Unknown";
  }
}

void imuBegin() {
  Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, IMU_I2C_FREQ);

  // Поиск на двух стандартных адресах
  const uint8_t addrs[] = { 0x68, 0x69 };
  for (uint8_t a : addrs) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { s_addr = a; break; }
  }

  if (s_addr == 0) {
    Serial.println("IMU: not found on I2C bus");
    return;
  }
  Serial.printf("IMU: found at 0x%02X\n", s_addr);

  uint8_t who = 0;
  if (i2cRead(s_addr, 0x75, &who, 1)) {
    g_imuWhoAmI = who;
    Serial.printf("IMU: WHO_AM_I = 0x%02X (%s)\n", who, imuName());
  }

  // Wake up
  i2cWrite(s_addr, 0x6B, 0x00);     // PWR_MGMT_1 = 0 (wake, internal clock)
  delay(50);
  i2cWrite(s_addr, 0x1C, 0x00);     // ACCEL_CONFIG = ±2g
  i2cWrite(s_addr, 0x1A, 0x03);     // CONFIG: DLPF ≈ 44 Hz

  g_imuFound = true;
  g_imuValid = true;

  // ─── Опциональная калибровка ───
  if (IMU_CALIBRATE_ON_BOOT) {
    Serial.println("IMU: calibrating (keep device flat)...");
    float sumX = 0.0f, sumY = 0.0f;
    int n = 0;
    for (int i = 0; i < 30; i++) {
      uint8_t buf[6];
      if (i2cRead(s_addr, 0x3B, buf, 6)) {
        int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
        int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
        sumX += ax / 16384.0f;
        sumY += ay / 16384.0f;
        n++;
      }
      delay(30);
    }
    if (n > 0) {
      s_offX = sumX / n;
      s_offY = sumY / n;
      Serial.printf("IMU: offsets = (%.3f, %.3f) g\n", s_offX, s_offY);
    }
  }
}

void imuUpdate() {
  if (!g_imuValid) return;

  uint8_t buf[6];
  if (!i2cRead(s_addr, 0x3B, buf, 6)) {
    if (++s_failCount > 20) {
      g_imuValid = false;
      Serial.println("IMU: too many read failures, disabling");
    }
    return;
  }
  s_failCount = 0;

  int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t az = (int16_t)((buf[4] << 8) | buf[5]);

  g_accelX = ax / 16384.0f - s_offX;
  g_accelY = ay / 16384.0f - s_offY;
  g_accelZ = az / 16384.0f;
}