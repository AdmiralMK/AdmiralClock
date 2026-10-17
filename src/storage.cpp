#include "storage.h"
#include <Preferences.h>

// Один namespace на всё приложение
static Preferences prefs;
static const char* NVS_NAMESPACE = "olyaclock";

// Ключи
static const char* KEY_THEME = "theme";

void storageBegin() {
  // open(namespace, readOnly=false) — создаст namespace, если его нет
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("NVS: failed to open namespace");
  } else {
    Serial.println("NVS: ready");
  }
}

int storageLoadTheme(int fallback) {
  int v = prefs.getInt(KEY_THEME, fallback);
  Serial.printf("NVS: loaded theme=%d\n", v);
  return v;
}

void storageSaveTheme(int idx) {
  prefs.putInt(KEY_THEME, idx);
  Serial.printf("NVS: saved theme=%d\n", idx);
}