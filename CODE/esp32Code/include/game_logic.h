#pragma once

#include "card_manager.h"
#include "game_types.h"

struct ActionContext {
  WaitReason waitReason = WaitReason::NONE;
  bool noCancel = false;
  uint8_t propertyId = 0;
  uint8_t eventId = 0;
  uint8_t debtorId = 0;
  uint8_t creditorId = 0;
  int32_t debtAmount = 0;
  int32_t auctionBid = 0;
  int32_t auctionSecondsLeft = 0;
  bool auctionAwaitWinner = false;
  char flash[48] = {0};
};

class GameLogic {
 public:
  void begin();

  UiState state() const { return state_; }
  const ActionContext &context() const { return ctx_; }
  const PlayerState *players() const { return players_; }
  const PropertyState *properties() const { return properties_; }

  void onBtn1();
  void onBtn2();
  void onBtn3();
  void triggerMenuAction(uint8_t action);
  void primePlayer(uint8_t playerId, int32_t balance);
  void onTick();

  void onPlayerCard(const CardTap &tap, CardManager &cards);
  void onPropertyCard(const CardTap &tap, CardManager &cards);
  void onEventCard(const CardTap &tap, CardManager &cards);

  bool isDirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }

 private:
  void setState(UiState next);
  void touchState();
  void ensurePlayer(uint8_t playerId);
  PlayerState *playerById(uint8_t playerId);
  PropertyState *propertyById(uint8_t propertyId);
  uint16_t propertyPrice(uint8_t propertyId) const;
  uint16_t propertyRent(uint8_t propertyId, uint8_t level) const;
  void enterDebt(uint8_t debtorId, uint8_t creditorId, int32_t amount);
  void settleDebtWithProperty(uint8_t propertyId);
  void resolveWinner();
  uint8_t firstOwnedProperty(uint8_t playerId);
  void applyEventToPlayer(const EventCardData &event, uint8_t playerId);
  void startAuction(uint8_t propertyId);
  void markPropertyDirty(uint8_t propertyId);

  UiState state_ = UiState::HOME;
  ActionContext ctx_{};
  PlayerState players_[GAME_MAX_PLAYERS]{};
  PropertyState properties_[PROPERTY_COUNT]{};
  bool dirty_ = true;
  uint32_t stateSinceMs_ = 0;
  uint32_t lastAuctionTickMs_ = 0;
  bool propertyDirty_[PROPERTY_COUNT]{};
};
