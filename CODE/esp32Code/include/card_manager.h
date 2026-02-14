#pragma once

#include <Adafruit_PN532.h>

#include "config.h"
#include "game_types.h"

struct PlayerCardData {
  uint8_t playerId = 0;
  int32_t balance = 1500;
  uint8_t jailed = 0;
  uint8_t bankrupt = 0;
};

struct PropertyCardData {
  uint8_t propertyId = 0;
  uint8_t ownerId = 0;
  uint8_t level = 1;
  uint16_t basePrice = 0;
};

struct EventCardData {
  uint8_t eventId = 0;
  EventType type = EventType::MONEY;
  int16_t value = 0;
};

class CardManager {
 public:
  explicit CardManager(Adafruit_PN532 &nfc) : nfc_(nfc) {}

  NfcCardType detectType(const CardTap &tap);

  bool readPlayer(const CardTap &tap, PlayerCardData &out);
  bool writePlayer(const CardTap &tap, const PlayerCardData &data);

  bool readProperty(const CardTap &tap, PropertyCardData &out);
  bool writeProperty(const CardTap &tap, const PropertyCardData &data);

  bool readEvent(const CardTap &tap, EventCardData &out);
  bool writeEvent(const CardTap &tap, const EventCardData &data);

 private:
  bool auth(const CardTap &tap, uint8_t block);
  bool readBlock(uint8_t block, uint8_t out[16]);
  bool writeBlock(uint8_t block, const uint8_t in[16]);

  Adafruit_PN532 &nfc_;
};
