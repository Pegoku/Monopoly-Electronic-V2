#include "nfc_manager.h"

#include <Wire.h>

bool NfcManager::begin() {
  Wire.begin(PIN_NFC_SDA, PIN_NFC_SCL);
  nfc_.begin();
  uint32_t version = nfc_.getFirmwareVersion();
  if (!version) {
    return false;
  }
  nfc_.SAMConfig();
  return true;
}

bool NfcManager::poll(CardTap &tap) {
  tap.valid = false;

  const uint32_t now = millis();
  if (now - lastPollMs_ < NFC_POLL_MS) {
    return false;
  }
  lastPollMs_ = now;

  uint8_t uid[7] = {0};
  uint8_t uidLen = 0;
  if (!nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 25)) {
    return false;
  }

  if (uidLen == lastUidLen_ && memcmp(uid, lastUid_, uidLen) == 0 && (now - lastSeenMs_) < CARD_DEBOUNCE_MS) {
    return false;
  }

  memcpy(lastUid_, uid, uidLen);
  lastUidLen_ = uidLen;
  lastSeenMs_ = now;

  tap.valid = true;
  tap.uidLen = uidLen;
  memcpy(tap.uid, uid, uidLen);
  return true;
}
