#include "card_manager.h"

namespace {
constexpr uint8_t KEY_A[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr uint8_t PLAYER_BLOCK = 4;
constexpr uint8_t PROPERTY_BLOCK = 8;
constexpr uint8_t EVENT_BLOCK = 12;
constexpr uint8_t MAGIC0 = 'M';
constexpr uint8_t MAGIC1 = 'B';
constexpr uint8_t MAGIC2 = '2';
constexpr uint8_t VERSION = 1;
}  // namespace

bool CardManager::auth(const CardTap &tap, uint8_t block) {
  return nfc_.mifareclassic_AuthenticateBlock(const_cast<uint8_t *>(tap.uid), tap.uidLen, block, 0, const_cast<uint8_t *>(KEY_A));
}

bool CardManager::readBlock(uint8_t block, uint8_t out[16]) {
  return nfc_.mifareclassic_ReadDataBlock(block, out);
}

bool CardManager::writeBlock(uint8_t block, const uint8_t in[16]) {
  return nfc_.mifareclassic_WriteDataBlock(block, const_cast<uint8_t *>(in));
}

NfcCardType CardManager::detectType(const CardTap &tap) {
  uint8_t block[16] = {0};

  if (auth(tap, PLAYER_BLOCK) && readBlock(PLAYER_BLOCK, block)) {
    if (block[0] == MAGIC0 && block[1] == MAGIC1 && block[2] == MAGIC2 && block[4] == static_cast<uint8_t>(NfcCardType::PLAYER)) {
      return NfcCardType::PLAYER;
    }
  }
  if (auth(tap, PROPERTY_BLOCK) && readBlock(PROPERTY_BLOCK, block)) {
    if (block[0] == MAGIC0 && block[1] == MAGIC1 && block[2] == MAGIC2 && block[4] == static_cast<uint8_t>(NfcCardType::PROPERTY)) {
      return NfcCardType::PROPERTY;
    }
  }
  if (auth(tap, EVENT_BLOCK) && readBlock(EVENT_BLOCK, block)) {
    if (block[0] == MAGIC0 && block[1] == MAGIC1 && block[2] == MAGIC2 && block[4] == static_cast<uint8_t>(NfcCardType::EVENT)) {
      return NfcCardType::EVENT;
    }
  }
  return NfcCardType::UNKNOWN;
}

bool CardManager::readPlayer(const CardTap &tap, PlayerCardData &out) {
  uint8_t b[16] = {0};
  if (!auth(tap, PLAYER_BLOCK) || !readBlock(PLAYER_BLOCK, b)) return false;
  if (b[0] != MAGIC0 || b[1] != MAGIC1 || b[2] != MAGIC2 || b[4] != static_cast<uint8_t>(NfcCardType::PLAYER)) return false;

  out.playerId = b[5];
  out.balance = static_cast<int32_t>(b[6]) | (static_cast<int32_t>(b[7]) << 8) | (static_cast<int32_t>(b[8]) << 16) | (static_cast<int32_t>(b[9]) << 24);
  out.jailed = b[10];
  out.bankrupt = b[11];
  return true;
}

bool CardManager::writePlayer(const CardTap &tap, const PlayerCardData &data) {
  uint8_t b[16] = {0};
  b[0] = MAGIC0;
  b[1] = MAGIC1;
  b[2] = MAGIC2;
  b[3] = VERSION;
  b[4] = static_cast<uint8_t>(NfcCardType::PLAYER);
  b[5] = data.playerId;
  b[6] = static_cast<uint8_t>(data.balance & 0xFF);
  b[7] = static_cast<uint8_t>((data.balance >> 8) & 0xFF);
  b[8] = static_cast<uint8_t>((data.balance >> 16) & 0xFF);
  b[9] = static_cast<uint8_t>((data.balance >> 24) & 0xFF);
  b[10] = data.jailed;
  b[11] = data.bankrupt;
  if (!auth(tap, PLAYER_BLOCK)) return false;
  return writeBlock(PLAYER_BLOCK, b);
}

bool CardManager::readProperty(const CardTap &tap, PropertyCardData &out) {
  uint8_t b[16] = {0};
  if (!auth(tap, PROPERTY_BLOCK) || !readBlock(PROPERTY_BLOCK, b)) return false;
  if (b[0] != MAGIC0 || b[1] != MAGIC1 || b[2] != MAGIC2 || b[4] != static_cast<uint8_t>(NfcCardType::PROPERTY)) return false;

  out.propertyId = b[5];
  out.ownerId = b[6];
  out.level = b[7] == 0 ? 1 : b[7];
  out.basePrice = static_cast<uint16_t>(b[8]) | (static_cast<uint16_t>(b[9]) << 8);
  return true;
}

bool CardManager::writeProperty(const CardTap &tap, const PropertyCardData &data) {
  uint8_t b[16] = {0};
  b[0] = MAGIC0;
  b[1] = MAGIC1;
  b[2] = MAGIC2;
  b[3] = VERSION;
  b[4] = static_cast<uint8_t>(NfcCardType::PROPERTY);
  b[5] = data.propertyId;
  b[6] = data.ownerId;
  b[7] = data.level;
  b[8] = static_cast<uint8_t>(data.basePrice & 0xFF);
  b[9] = static_cast<uint8_t>((data.basePrice >> 8) & 0xFF);
  if (!auth(tap, PROPERTY_BLOCK)) return false;
  return writeBlock(PROPERTY_BLOCK, b);
}

bool CardManager::readEvent(const CardTap &tap, EventCardData &out) {
  uint8_t b[16] = {0};
  if (!auth(tap, EVENT_BLOCK) || !readBlock(EVENT_BLOCK, b)) return false;
  if (b[0] != MAGIC0 || b[1] != MAGIC1 || b[2] != MAGIC2 || b[4] != static_cast<uint8_t>(NfcCardType::EVENT)) return false;

  out.eventId = b[5];
  out.type = static_cast<EventType>(b[6]);
  out.value = static_cast<int16_t>(b[7]) | (static_cast<int16_t>(b[8]) << 8);
  return true;
}

bool CardManager::writeEvent(const CardTap &tap, const EventCardData &data) {
  uint8_t b[16] = {0};
  b[0] = MAGIC0;
  b[1] = MAGIC1;
  b[2] = MAGIC2;
  b[3] = VERSION;
  b[4] = static_cast<uint8_t>(NfcCardType::EVENT);
  b[5] = data.eventId;
  b[6] = static_cast<uint8_t>(data.type);
  b[7] = static_cast<uint8_t>(data.value & 0xFF);
  b[8] = static_cast<uint8_t>((data.value >> 8) & 0xFF);
  if (!auth(tap, EVENT_BLOCK)) return false;
  return writeBlock(EVENT_BLOCK, b);
}
