#pragma once
#include <Arduino.h>

// Состояние угрозы
enum AlertState {
  ALERT_NONE,       // нет активных событий
  ALERT_DRONE,      // беспилотная опасность
  ALERT_ROCKET,     // ракетная опасность
  CANCEL_DRONE,     // отмена беспилотной
  CANCEL_ROCKET     // отмена ракетной
};

extern AlertState g_alertState;
extern unsigned long g_alertStartMs;  // время установки текущего состояния (millis)

void newsMonitorBegin();
void newsMonitorUpdate();

extern bool g_manualOverride;
void newsMonitorHandleSerial();