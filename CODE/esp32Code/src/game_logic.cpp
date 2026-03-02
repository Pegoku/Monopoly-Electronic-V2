#include "game_logic.h"

#include <stdio.h>
#include <string.h>

namespace {
constexpr uint8_t kMaxLevel = 5;

constexpr uint8_t kPropertyTableCount = 22;
constexpr uint16_t kPropertyLevelValues[kPropertyTableCount][kMaxLevel] = {
    {70, 130, 220, 370, 750},
    {70, 130, 220, 370, 750},
    {80, 140, 240, 410, 800},
    {80, 140, 240, 410, 800},
    {100, 160, 260, 440, 860},
    {110, 180, 290, 460, 900},
    {110, 180, 290, 460, 900},
    {130, 200, 310, 490, 980},
    {140, 210, 330, 520, 1000},
    {140, 210, 330, 520, 1000},
    {160, 230, 350, 550, 1100},
    {170, 250, 380, 580, 1160},
    {170, 250, 380, 580, 1160},
    {190, 270, 400, 610, 1200},
    {200, 280, 420, 640, 1300},
    {200, 280, 420, 640, 1300},
    {220, 300, 440, 670, 1340},
    {230, 320, 460, 700, 1400},
    {230, 320, 460, 700, 1400},
    {250, 340, 480, 730, 1440},
    {270, 360, 510, 740, 1500},
    {300, 400, 560, 810, 1600},
};

uint16_t fallbackPropertyBasePrice(uint8_t propertyId) {
  return 80 + static_cast<uint16_t>(propertyId * 12.5f + 0.5f);
}

uint16_t propertyValueByLevel(uint8_t propertyId, uint8_t level) {
  if (level < 1) level = 1;
  if (level > kMaxLevel) level = kMaxLevel;
  if (propertyId >= 1 && propertyId <= kPropertyTableCount) {
    return kPropertyLevelValues[propertyId - 1][level - 1];
  }

  const uint16_t basePrice = fallbackPropertyBasePrice(propertyId);
  return basePrice * level;
}

EventCardData makeEvent(uint8_t id, EventType type, int16_t value) {
  EventCardData e;
  e.eventId = id;
  e.type = type;
  e.value = value;
  return e;
}

const EventCardData kDefaultEvents[] = {
    makeEvent(1, EventType::MONEY, 200),
    makeEvent(2, EventType::MONEY, -150),
    makeEvent(3, EventType::JAIL, 0),
    makeEvent(4, EventType::RENT_BOOST, 1),
    makeEvent(5, EventType::MONEY, 100),
    makeEvent(6, EventType::MONEY, -200),
};

void setFlash(ActionContext &ctx, const char *text) {
  strncpy(ctx.flash, text, sizeof(ctx.flash) - 1);
  ctx.flash[sizeof(ctx.flash) - 1] = '\0';
}
}  // namespace

void GameLogic::begin() {
  for (uint8_t i = 0; i < PROPERTY_COUNT; i++) {
    const uint8_t id = i + 1;
    const uint16_t price = propertyValueByLevel(id, 1);
    properties_[i].id = id;
    properties_[i].ownerId = 0;
    properties_[i].level = 1;
    properties_[i].basePrice = price;
    properties_[i].baseRent = price;
  }
  for (uint8_t i = 0; i < PROPERTY_COUNT; i++) {
    propertyDirty_[i] = false;
  }
  setState(UiState::HOME);
}

void GameLogic::markPropertyDirty(uint8_t propertyId) {
  (void)propertyId;
}

void GameLogic::setState(UiState next) {
  state_ = next;
  stateSinceMs_ = millis();
  if (state_ == UiState::AUCTION) {
    lastAuctionTickMs_ = millis();
  }
  dirty_ = true;
}

void GameLogic::touchState() {
  dirty_ = true;
}

void GameLogic::ensurePlayer(uint8_t playerId) {
  if (playerId < 1 || playerId > GAME_MAX_PLAYERS) return;
  PlayerState &p = players_[playerId - 1];
  if (!p.active) {
    p.active = true;
    p.id = playerId;
    p.balance = 1500;
  }
}

PlayerState *GameLogic::playerById(uint8_t playerId) {
  if (playerId < 1 || playerId > GAME_MAX_PLAYERS) return nullptr;
  PlayerState &p = players_[playerId - 1];
  return p.active ? &p : nullptr;
}

PropertyState *GameLogic::propertyById(uint8_t propertyId) {
  if (propertyId < 1 || propertyId > PROPERTY_COUNT) return nullptr;
  return &properties_[propertyId - 1];
}

uint16_t GameLogic::propertyPrice(uint8_t propertyId) const {
  if (propertyId < 1 || propertyId > PROPERTY_COUNT) return 0;
  return properties_[propertyId - 1].basePrice;
}

uint16_t GameLogic::propertyRent(uint8_t propertyId, uint8_t level) const {
  if (propertyId < 1 || propertyId > PROPERTY_COUNT) return 0;
  return propertyValueByLevel(propertyId, level);
}

void GameLogic::enterDebt(uint8_t debtorId, uint8_t creditorId, int32_t amount) {
  ctx_ = {};
  ctx_.debtorId = debtorId;
  ctx_.creditorId = creditorId;
  ctx_.debtAmount = amount;
  setFlash(ctx_, "DEBT");
  setState(UiState::DEBT);
}

uint8_t GameLogic::firstOwnedProperty(uint8_t playerId) {
  for (uint8_t i = 0; i < PROPERTY_COUNT; i++) {
    if (properties_[i].ownerId == playerId) return properties_[i].id;
  }
  return 0;
}

void GameLogic::resolveWinner() {
  uint8_t alive = 0;
  uint8_t winner = 0;
  for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
    if (players_[i].active && !players_[i].bankrupt) {
      alive++;
      winner = players_[i].id;
    }
  }
  if (alive == 1) {
    ctx_ = {};
    ctx_.debtorId = winner;
    setState(UiState::WINNER);
  }
}

void GameLogic::settleDebtWithProperty(uint8_t propertyId) {
  PropertyState *prop = propertyById(propertyId);
  if (!prop) return;
  if (prop->ownerId != ctx_.debtorId) return;

  prop->ownerId = ctx_.creditorId;
  if (ctx_.creditorId == 0) prop->level = 1;
  markPropertyDirty(prop->id);

  ctx_.debtAmount -= prop->basePrice;
  if (ctx_.debtAmount <= 0) {
    ctx_ = {};
    setFlash(ctx_, "DEBT CLEARED");
    setState(UiState::HOME);
    return;
  }

  PlayerState *debtor = playerById(ctx_.debtorId);
  if (!debtor) return;
  if (debtor->balance <= 0 && firstOwnedProperty(debtor->id) == 0) {
    debtor->bankrupt = true;
    resolveWinner();
    if (state_ != UiState::WINNER) {
      ctx_ = {};
      setFlash(ctx_, "BANKRUPT");
      setState(UiState::HOME);
    }
  }
  touchState();
}

void GameLogic::applyEventToPlayer(const EventCardData &event, uint8_t playerId) {
  PlayerState *p = playerById(playerId);
  if (!p || p->bankrupt) return;

  if (event.type == EventType::MONEY) {
    if (event.value >= 0) {
      p->balance += event.value;
      ctx_ = {};
      setFlash(ctx_, "+MONEY");
      setState(UiState::HOME);
      return;
    }
    const int32_t owed = -event.value;
    if (p->balance >= owed) {
      p->balance -= owed;
      ctx_ = {};
      setFlash(ctx_, "-MONEY");
      setState(UiState::HOME);
      return;
    }
    const int32_t left = owed - p->balance;
    p->balance = 0;
    enterDebt(p->id, 0, left);
    return;
  }

  if (event.type == EventType::JAIL) {
    p->jailed = true;
    ctx_ = {};
    setFlash(ctx_, "GO JAIL");
    setState(UiState::HOME);
    return;
  }

  if (event.type == EventType::RENT_BOOST) {
    const uint8_t owned = firstOwnedProperty(p->id);
    PropertyState *prop = propertyById(owned);
    if (prop) {
      if (prop->level < kMaxLevel) prop->level++;
      markPropertyDirty(prop->id);
    }
    ctx_ = {};
    setFlash(ctx_, "RENT +1");
    setState(UiState::HOME);
  }
}

void GameLogic::startAuction(uint8_t propertyId) {
  ctx_ = {};
  ctx_.propertyId = propertyId;
  ctx_.auctionBid = 0;
  ctx_.auctionSecondsLeft = 10;
  ctx_.auctionAwaitWinner = false;
  setState(UiState::AUCTION);
}

void GameLogic::primePlayer(uint8_t playerId, int32_t balance) {
  ensurePlayer(playerId);
  PlayerState *p = playerById(playerId);
  if (!p) return;
  p->balance = balance;
  p->jailed = false;
  p->bankrupt = false;
  touchState();
}

void GameLogic::onBtn1() {
  if (state_ == UiState::HOME) {
    ctx_ = {};
    setState(UiState::GO);
    return;
  }
  if (state_ == UiState::PROPERTY_UNOWNED) {
    ctx_.waitReason = WaitReason::BUY_PLAYER;
    ctx_.noCancel = false;
    setState(UiState::WAIT_CARD);
    return;
  }
  if (state_ == UiState::PROPERTY_OWNED) {
    ctx_.waitReason = WaitReason::RENT_PAYER;
    ctx_.noCancel = false;
    setState(UiState::WAIT_CARD);
    return;
  }
  if (state_ == UiState::AUCTION && !ctx_.auctionAwaitWinner) {
    ctx_.auctionBid += 20;
    touchState();
  }
}

void GameLogic::onBtn2() {
  if (state_ == UiState::WAIT_CARD && ctx_.noCancel) {
    return;
  }
  if (state_ == UiState::WINNER) {
    return;
  }
  ctx_ = {};
  setState(UiState::HOME);
}

void GameLogic::onBtn3() {
  if (state_ == UiState::HOME) {
    ctx_ = {};
    setState(UiState::JAIL);
    return;
  }
  if (state_ == UiState::PROPERTY_UNOWNED) {
    startAuction(ctx_.propertyId);
  }
}

void GameLogic::triggerMenuAction(uint8_t action) {
  if (state_ != UiState::HOME) {
    return;
  }
  ctx_ = {};
  if (action == 0) {
    setState(UiState::GO);
  } else if (action == 1) {
    setState(UiState::JAIL);
  } else if (action == 2) {
    setState(UiState::TRAIN);
  }
}

void GameLogic::onTick() {
  const uint32_t now = millis();

  if (state_ == UiState::WAIT_CARD || state_ == UiState::GO || state_ == UiState::TRAIN || state_ == UiState::JAIL) {
    if (now - stateSinceMs_ > WAIT_TIMEOUT_MS) {
      ctx_ = {};
      setFlash(ctx_, "TIMEOUT");
      setState(UiState::HOME);
      return;
    }
  }

  if (state_ == UiState::EVENT) {
    if (now - stateSinceMs_ > 800) {
      ctx_.waitReason = WaitReason::EVENT_TARGET;
      ctx_.noCancel = true;
      setState(UiState::WAIT_CARD);
    }
    return;
  }

  if (state_ == UiState::AUCTION && !ctx_.auctionAwaitWinner) {
    if (now - lastAuctionTickMs_ >= 1000) {
      lastAuctionTickMs_ = now;
      if (ctx_.auctionSecondsLeft > 0) {
        ctx_.auctionSecondsLeft--;
        touchState();
      }
      if (ctx_.auctionSecondsLeft <= 0) {
        ctx_.auctionAwaitWinner = true;
        touchState();
      }
    }
  }
}

void GameLogic::onPlayerCard(const CardTap &tap, CardManager &cards) {
  PlayerCardData card{};
  if (!cards.readPlayer(tap, card)) {
    return;
  }

  ensurePlayer(card.playerId);
  PlayerState *player = playerById(card.playerId);
  if (!player) return;

  if (state_ == UiState::GO) {
    player->balance += 200;
    ctx_ = {};
    setFlash(ctx_, "+200");
    setState(UiState::HOME);
    return;
  }

  if (state_ == UiState::JAIL) {
    if (player->balance >= 100) {
      player->balance -= 100;
      player->jailed = false;
      ctx_ = {};
      setFlash(ctx_, "JAIL -100");
      setState(UiState::HOME);
    } else {
      const int32_t left = 100 - player->balance;
      player->balance = 0;
      enterDebt(player->id, 0, left);
    }
    return;
  }

  if (state_ == UiState::TRAIN) {
    if (player->balance >= 100) {
      player->balance -= 100;
      ctx_ = {};
      setFlash(ctx_, "TRAIN -100");
      setState(UiState::HOME);
    } else {
      const int32_t left = 100 - player->balance;
      player->balance = 0;
      enterDebt(player->id, 0, left);
    }
    return;
  }

  if (state_ == UiState::WAIT_CARD && ctx_.waitReason == WaitReason::BUY_PLAYER) {
    PropertyState *prop = propertyById(ctx_.propertyId);
    if (!prop) return;
    const int32_t price = prop->basePrice;
    if (player->balance >= price) {
      player->balance -= price;
      prop->ownerId = player->id;
      prop->level = 1;
      markPropertyDirty(prop->id);
      ctx_ = {};
      setFlash(ctx_, "PURCHASE");
      setState(UiState::HOME);
    } else {
      const int32_t left = price - player->balance;
      player->balance = 0;
      enterDebt(player->id, 0, left);
    }
    return;
  }

  if (state_ == UiState::WAIT_CARD && ctx_.waitReason == WaitReason::RENT_PAYER) {
    PropertyState *prop = propertyById(ctx_.propertyId);
    if (!prop || prop->ownerId == 0) return;

    if (player->id == prop->ownerId) {
      if (prop->level < kMaxLevel) prop->level++;
      markPropertyDirty(prop->id);
      ctx_ = {};
      setFlash(ctx_, "LEVEL UP");
      setState(UiState::HOME);
      return;
    }

    const int32_t rent = propertyRent(prop->id, prop->level);
    PlayerState *owner = playerById(prop->ownerId);
    if (!owner) return;

    if (player->balance >= rent) {
      player->balance -= rent;
      owner->balance += rent;
      if (prop->level < kMaxLevel) prop->level++;
      markPropertyDirty(prop->id);
      ctx_ = {};
      setFlash(ctx_, "RENT PAID");
      setState(UiState::HOME);
    } else {
      owner->balance += player->balance;
      const int32_t left = rent - player->balance;
      player->balance = 0;
      enterDebt(player->id, owner->id, left);
    }
    return;
  }

  if (state_ == UiState::WAIT_CARD && ctx_.waitReason == WaitReason::EVENT_TARGET) {
    EventCardData event = kDefaultEvents[0];
    for (const auto &e : kDefaultEvents) {
      if (e.eventId == ctx_.eventId) {
        event = e;
        break;
      }
    }
    applyEventToPlayer(event, player->id);
    return;
  }

  if (state_ == UiState::AUCTION && ctx_.auctionAwaitWinner) {
    PropertyState *prop = propertyById(ctx_.propertyId);
    if (!prop) return;
    const int32_t bid = ctx_.auctionBid;
    if (player->balance >= bid) {
      player->balance -= bid;
      prop->ownerId = player->id;
      prop->level = 1;
      markPropertyDirty(prop->id);
      ctx_ = {};
      setFlash(ctx_, "AUCTION OK");
      setState(UiState::HOME);
    } else {
      const int32_t left = bid - player->balance;
      player->balance = 0;
      enterDebt(player->id, 0, left);
    }
    return;
  }

  if (state_ == UiState::HOME) {
    ctx_ = {};
    setFlash(ctx_, "CARD");
    touchState();
  }
}

void GameLogic::onPropertyCard(const CardTap &tap, CardManager &cards) {
  (void)cards;
  PropertyCardData card{};
  if (!cards.readProperty(tap, card)) {
    return;
  }

  PropertyState *prop = propertyById(card.propertyId);
  if (!prop) return;

  if (card.basePrice > 0) {
    prop->basePrice = card.basePrice;
    prop->baseRent = card.basePrice;
  }

  if (state_ == UiState::DEBT) {
    settleDebtWithProperty(prop->id);
    return;
  }

  if (state_ != UiState::HOME && state_ != UiState::WAIT_CARD) {
    return;
  }

  ctx_ = {};
  ctx_.propertyId = prop->id;
  if (prop->ownerId == 0) {
    setState(UiState::PROPERTY_UNOWNED);
  } else {
    setState(UiState::PROPERTY_OWNED);
  }
}

void GameLogic::onEventCard(const CardTap &tap, CardManager &cards) {
  if (state_ != UiState::HOME) return;

  EventCardData card{};
  if (!cards.readEvent(tap, card)) {
    return;
  }

  ctx_ = {};
  ctx_.eventId = card.eventId;
  setState(UiState::EVENT);
}
