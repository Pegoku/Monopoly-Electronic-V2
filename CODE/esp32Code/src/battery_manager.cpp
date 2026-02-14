#include "battery_manager.h"

#include <Arduino.h>

#include "config.h"

void BatteryManager::begin() {
  analogReadResolution(12);
}

float BatteryManager::readPercent() {
  const int raw = analogRead(PIN_BATTERY_ADC);
  const float mv = (raw / 4095.0f) * 3300.0f * 2.0f;
  const float pct = (mv - 3300.0f) / (4200.0f - 3300.0f) * 100.0f;
  if (pct < 0.0f) return 0.0f;
  if (pct > 100.0f) return 100.0f;
  return pct;
}
