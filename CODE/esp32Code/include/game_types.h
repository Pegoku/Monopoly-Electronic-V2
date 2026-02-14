#pragma once

#include <Arduino.h>

enum class UiState : uint8_t {
  HOME,
  WAIT_CARD,
  PROPERTY_UNOWNED,
  PROPERTY_OWNED,
  EVENT,
  AUCTION,
  DEBT,
  GO,
  JAIL,
  WINNER
};

enum class WaitReason : uint8_t {
  NONE,
  BUY_PLAYER,
  RENT_PAYER,
  EVENT_TARGET
};

enum class NfcCardType : uint8_t {
  UNKNOWN = 0,
  PLAYER = 1,
  PROPERTY = 2,
  EVENT = 3
};

enum class EventType : uint8_t {
  MONEY = 1,
  JAIL = 2,
  RENT_BOOST = 3
};

struct PlayerState {
  bool active = false;
  uint8_t id = 0;
  int32_t balance = 1500;
  bool jailed = false;
  bool bankrupt = false;
};

struct PropertyState {
  uint8_t id = 0;
  uint8_t ownerId = 0;
  uint8_t level = 1;
  uint16_t basePrice = 0;
  uint16_t baseRent = 0;
};

struct EventCard {
  uint8_t id = 0;
  EventType type = EventType::MONEY;
  int16_t value = 0;
};

struct CardTap {
  bool valid = false;
  NfcCardType type = NfcCardType::UNKNOWN;
  uint8_t uid[7] = {0};
  uint8_t uidLen = 0;
};

constexpr uint8_t GAME_MAX_PLAYERS = 8;
constexpr uint8_t PROPERTY_COUNT = 28;
