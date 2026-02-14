#pragma once

#include <Adafruit_PN532.h>
#include <Wire.h>

#include "config.h"
#include "game_types.h"

class NfcManager {
 public:
  bool begin();
 bool poll(CardTap &tap);
  Adafruit_PN532 &driver() { return nfc_; }

 private:
  Adafruit_PN532 nfc_{static_cast<uint8_t>(PIN_NFC_IRQ), static_cast<uint8_t>(PIN_NFC_RST), &Wire};
  uint8_t lastUid_[7] = {0};
  uint8_t lastUidLen_ = 0;
  uint32_t lastSeenMs_ = 0;
  uint32_t lastPollMs_ = 0;
};
