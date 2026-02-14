#include "game_logic.h"

#include <stdio.h>
#include <string.h>

namespace {
constexpr uint8_t kMaxLevel = 5;

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
    const uint16_t price = 80 + static_cast<uint16_t>(id * 12.5f + 0.5f);
    properties_[i].id = id;
    properties_[i].ownerId = 0;
    properties_[i].level = 1;
    properties_[i].basePrice = price;
    properties_[i].baseRent = price / 4;
    if (properties_[i].baseRent < 20) properties_[i].baseRent = 20;
  }
  setState(UiState::HOME);
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
  if (level < 1) level = 1;
  if (level > kMaxLevel) level = kMaxLevel;
  return properties_[propertyId - 1].baseRent * level;
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

void GameLogic::onTick() {
  const uint32_t now = millis();

  if (state_ == UiState::WAIT_CARD || state_ == UiState::GO || state_ == UiState::JAIL) {
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

  player->jailed = card.jailed > 0;
  player->bankrupt = card.bankrupt > 0;
  player->balance = card.balance;

  if (state_ == UiState::GO) {
    player->balance += 200;
    card.balance = player->balance;
    cards.writePlayer(tap, card);
    ctx_ = {};
    setFlash(ctx_, "+200");
    setState(UiState::HOME);
    return;
  }

  if (state_ == UiState::JAIL) {
    if (player->balance >= 100) {
      player->balance -= 100;
      player->jailed = false;
      card.balance = player->balance;
      card.jailed = 0;
      cards.writePlayer(tap, card);
      ctx_ = {};
      setFlash(ctx_, "JAIL -100");
      setState(UiState::HOME);
    } else {
      const int32_t left = 100 - player->balance;
      player->balance = 0;
      card.balance = 0;
      cards.writePlayer(tap, card);
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
      card.balance = player->balance;
      cards.writePlayer(tap, card);
      ctx_ = {};
      setFlash(ctx_, "PURCHASE");
      setState(UiState::HOME);
    } else {
      const int32_t left = price - player->balance;
      player->balance = 0;
      card.balance = 0;
      cards.writePlayer(tap, card);
      enterDebt(player->id, 0, left);
    }
    return;
  }

  if (state_ == UiState::WAIT_CARD && ctx_.waitReason == WaitReason::RENT_PAYER) {
    PropertyState *prop = propertyById(ctx_.propertyId);
    if (!prop || prop->ownerId == 0) return;

    if (player->id == prop->ownerId) {
      if (prop->level < kMaxLevel) prop->level++;
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

      card.balance = player->balance;
      cards.writePlayer(tap, card);
      ctx_ = {};
      setFlash(ctx_, "RENT PAID");
      setState(UiState::HOME);
    } else {
      owner->balance += player->balance;
      const int32_t left = rent - player->balance;
      player->balance = 0;
      card.balance = 0;
      cards.writePlayer(tap, card);
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
    card.balance = player->balance;
    card.jailed = player->jailed ? 1 : 0;
    card.bankrupt = player->bankrupt ? 1 : 0;
    cards.writePlayer(tap, card);
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
      card.balance = player->balance;
      cards.writePlayer(tap, card);
      ctx_ = {};
      setFlash(ctx_, "AUCTION OK");
      setState(UiState::HOME);
    } else {
      const int32_t left = bid - player->balance;
      player->balance = 0;
      card.balance = 0;
      cards.writePlayer(tap, card);
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
  PropertyCardData card{};
  if (!cards.readProperty(tap, card)) {
    return;
  }

  PropertyState *prop = propertyById(card.propertyId);
  if (!prop) return;

  if (card.basePrice == 0) {
    card.basePrice = prop->basePrice;
    cards.writeProperty(tap, card);
  }

  prop->ownerId = card.ownerId;
  prop->level = card.level < 1 ? 1 : card.level;
  prop->basePrice = card.basePrice;
  prop->baseRent = card.basePrice / 4;
  if (prop->baseRent < 20) prop->baseRent = 20;

  if (state_ == UiState::DEBT) {
    settleDebtWithProperty(prop->id);
    card.ownerId = prop->ownerId;
    card.level = prop->level;
    cards.writeProperty(tap, card);
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
