#include "sound_manager.h"

#include <Arduino.h>

#include "config.h"

namespace {
constexpr uint8_t CH = 0;
}

void SoundManager::begin() {
  ledcSetup(CH, 2000, 8);
  ledcAttachPin(PIN_SPEAKER, CH);
  ledcWrite(CH, 0);
}

void SoundManager::beepOk() {
  ledcWriteTone(CH, 1600);
  delay(35);
  ledcWrite(CH, 0);
}

void SoundManager::beepError() {
  ledcWriteTone(CH, 380);
  delay(70);
  ledcWrite(CH, 0);
}

void SoundManager::beepTick() {
  ledcWriteTone(CH, 900);
  delay(12);
  ledcWrite(CH, 0);
}
