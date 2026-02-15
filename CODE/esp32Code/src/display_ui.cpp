#include "display_ui.h"

#include <SPI.h>

namespace {
constexpr uint16_t BG = ST77XX_WHITE;
constexpr uint16_t FG = ST77XX_BLACK;
constexpr uint16_t ACCENT = ST77XX_WHITE;
constexpr uint16_t SOFT = 0xBDF7;

const char *stateName(UiState state) {
  switch (state) {
    case UiState::HOME:
      return "HOME";
    case UiState::WAIT_CARD:
      return "WAIT";
    case UiState::PROPERTY_UNOWNED:
      return "UNOWNED";
    case UiState::PROPERTY_OWNED:
      return "OWNED";
    case UiState::EVENT:
      return "EVENT";
    case UiState::AUCTION:
      return "AUCTION";
    case UiState::DEBT:
      return "DEBT";
    case UiState::GO:
      return "GO";
    case UiState::TRAIN:
      return "TRAIN";
    case UiState::JAIL:
      return "JAIL";
    case UiState::WINNER:
      return "WINNER";
  }
  return "?";
}

const char *iconByPlayerId(uint8_t id) {
  static const char *icons[] = {"car", "ship", "plane", "hat", "boot", "dog", "cat", "iron"};
  if (id < 1 || id > 8) return "?";
  return icons[id - 1];
}

void drawTokenGlyph(Adafruit_ST7789 &tft, int16_t x, int16_t y, uint8_t playerId) {
  const uint8_t kind = (playerId - 1) % 4;
  if (kind == 0) {
    // car
    tft.drawRect(x + 1, y + 3, 10, 5, FG);
    tft.drawFastHLine(x + 3, y + 2, 6, FG);
    tft.fillCircle(x + 3, y + 9, 1, FG);
    tft.fillCircle(x + 9, y + 9, 1, FG);
    return;
  }
  if (kind == 1) {
    // boat
    tft.drawFastHLine(x + 1, y + 7, 10, FG);
    tft.drawLine(x + 1, y + 7, x + 4, y + 10, FG);
    tft.drawLine(x + 11, y + 7, x + 8, y + 10, FG);
    tft.drawFastVLine(x + 6, y + 1, 6, FG);
    tft.drawLine(x + 6, y + 1, x + 10, y + 4, FG);
    return;
  }
  if (kind == 2) {
    // plane
    tft.drawFastHLine(x + 1, y + 5, 10, FG);
    tft.drawLine(x + 5, y + 1, x + 7, y + 5, FG);
    tft.drawLine(x + 5, y + 9, x + 7, y + 5, FG);
    tft.drawPixel(x + 10, y + 4, FG);
    return;
  }
  // hat
  tft.drawFastHLine(x + 1, y + 9, 10, FG);
  tft.drawFastHLine(x + 3, y + 7, 6, FG);
  tft.drawFastHLine(x + 4, y + 5, 4, FG);
}
}  // namespace

bool DisplayUi::begin() {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  pinMode(PIN_TFT_CS, OUTPUT);
  pinMode(PIN_TFT_DC, OUTPUT);
  if (PIN_TFT_RST >= 0) {
    pinMode(PIN_TFT_RST, OUTPUT);
  }

  tft_.init(240, 320);
  tft_.setRotation(3);
  tft_.invertDisplay(false);

  tft_.fillScreen(ST77XX_RED);
  delay(120);
  tft_.fillScreen(ST77XX_GREEN);
  delay(120);
  tft_.fillScreen(ST77XX_BLUE);
  delay(120);
  tft_.fillScreen(BG);
  tft_.setTextWrap(false);
  return true;
}

void DisplayUi::clearMain() {
  tft_.fillRect(0, 0, SCREEN_W, SCREEN_H, BG);
}

void DisplayUi::drawStatusBar(UiState state, float batteryPercent) {
  tft_.fillRect(0, 0, SCREEN_W, 20, SOFT);
  tft_.drawFastHLine(0, 20, SCREEN_W, FG);
  tft_.setTextColor(FG);
  tft_.setTextSize(1);
  tft_.setCursor(6, 6);
  tft_.print("bank");
  tft_.setCursor(44, 6);
  tft_.print(stateName(state));
  tft_.setCursor(278, 6);
  tft_.print((int)batteryPercent);
  tft_.print('%');
}

void DisplayUi::drawHome(const GameLogic &game) {
  const PlayerState *players = game.players();

  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);

  tft_.setTextSize(2);
  int y = 40;
  bool any = false;
  for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
    if (!players[i].active || players[i].bankrupt) continue;
    any = true;
    drawTokenGlyph(tft_, 16, y + 4, players[i].id);
    tft_.setCursor(42, y);
    tft_.print(players[i].balance);
    y += 28;
    if (y > 176) break;
  }

  if (!any) {
    tft_.setTextSize(2);
    tft_.setCursor(12, 104);
    tft_.print("tap player card");
  }

  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("X:back   M:menu   Y:select");

  if (game.context().flash[0] != '\0') {
    tft_.setTextSize(1);
    tft_.setCursor(8, 228);
    tft_.print(game.context().flash);
  }
}

void DisplayUi::drawWaitCard(const ActionContext &ctx) {
  const int x = 116;
  const int y = 52;
  const int w = 86;
  const int h = 136;
  const int step = 8;
  const int dash = 4;
  const int phase = (millis() / 120) % 2;

  tft_.setTextSize(2);
  tft_.setTextColor(FG);
  tft_.setCursor(96, 30);
  tft_.print("TAP CARD");

  for (int i = 0; i < w; i += step) {
    if (((i / step) + phase) % 2 == 0) {
      tft_.drawFastHLine(x + i, y, dash, FG);
      tft_.drawFastHLine(x + i, y + h, dash, FG);
    }
  }
  for (int i = 0; i < h; i += step) {
    if (((i / step) + phase) % 2 == 0) {
      tft_.drawFastVLine(x, y + i, dash, FG);
      tft_.drawFastVLine(x + w, y + i, dash, FG);
    }
  }

  if (((millis() / 300) % 2) == 0) {
    tft_.setTextSize(1);
    tft_.setTextColor(FG);
    tft_.setCursor(304, 228);
    tft_.print("x");
  }

  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  if (ctx.waitReason == WaitReason::BUY_PLAYER) {
    tft_.print("waiting bank card for purchase");
  } else if (ctx.waitReason == WaitReason::RENT_PAYER) {
    tft_.print("waiting payer bank card");
  } else if (ctx.waitReason == WaitReason::EVENT_TARGET) {
    tft_.print("waiting target bank card");
  } else {
    tft_.print("waiting card...");
  }

  if (ctx.noCancel) {
    tft_.setTextSize(1);
    tft_.setCursor(8, 228);
    tft_.print("no cancel");
  }
}

void DisplayUi::drawPropertyUnowned(const GameLogic &game, const ActionContext &ctx) {
  const PropertyState *prop = &game.properties()[ctx.propertyId - 1];
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(1);
  tft_.setCursor(12, 32);
  tft_.print("unowned property");
  tft_.setTextSize(3);
  tft_.setCursor(20, 68);
  tft_.print(prop->id);
  tft_.setCursor(20, 122);
  tft_.print(prop->basePrice);
  tft_.setTextSize(2);
  tft_.setCursor(236, 68);
  tft_.print("house");
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("X back   M auction   Y buy");
}

void DisplayUi::drawPropertyOwned(const GameLogic &game, const ActionContext &ctx) {
  const PropertyState *prop = &game.properties()[ctx.propertyId - 1];
  const int rent = prop->baseRent * prop->level;
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(1);
  tft_.setCursor(12, 32);
  tft_.print("owned property");
  tft_.setTextSize(2);
  tft_.setCursor(10, 70);
  tft_.print(iconByPlayerId(prop->ownerId));
  tft_.setCursor(106, 70);
  tft_.print(rent);
  tft_.setCursor(10, 114);
  tft_.print(prop->id);
  tft_.setCursor(106, 114);
  tft_.print(prop->level);
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("X back   M menu   Y continue");
}

void DisplayUi::drawEvent(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(12, 68);
  tft_.print("event");
  tft_.setTextSize(3);
  tft_.setCursor(12, 112);
  tft_.print(ctx.eventId);
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("apply then tap bank card");
}

void DisplayUi::drawAuction(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 64);
  tft_.print("auction");
  tft_.setCursor(180, 64);
  tft_.print(ctx.auctionSecondsLeft);
  tft_.setTextSize(3);
  tft_.setCursor(10, 114);
  tft_.print(ctx.auctionBid);
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  if (ctx.auctionAwaitWinner) {
    tft_.print("winner tap bank card");
  } else {
    tft_.print("X back   M menu   Y +20");
  }
}

void DisplayUi::drawDebt(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 70);
  tft_.print("debt");
  tft_.setTextSize(3);
  tft_.setCursor(10, 120);
  tft_.print('-');
  tft_.print(ctx.debtAmount);
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("tap debtor property cards");
}

void DisplayUi::drawGo() {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 90);
  tft_.print("go");
  tft_.setTextSize(3);
  tft_.setCursor(10, 130);
  tft_.print("+200");
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("tap player bank card");
  tft_.setCursor(8, 228);
  tft_.print("X back   M menu   Y select");
}

void DisplayUi::drawTrain() {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 90);
  tft_.print("train");
  tft_.setTextSize(3);
  tft_.setCursor(10, 130);
  tft_.print("100");
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("tap player bank card");
  tft_.setCursor(8, 228);
  tft_.print("X back   M menu   Y select");
}

void DisplayUi::drawJail() {
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 90);
  tft_.print("jail");
  tft_.setTextSize(3);
  tft_.setCursor(10, 130);
  tft_.print("100");
  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("tap player bank card");
  tft_.setCursor(8, 228);
  tft_.print("X back   M menu   Y select");
}

void DisplayUi::drawWinner(const GameLogic &game, const ActionContext &ctx) {
  const PlayerState *winner = &game.players()[ctx.debtorId - 1];
  tft_.setTextColor(FG);
  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 80);
  tft_.print(iconByPlayerId(winner->id));
  tft_.setTextSize(3);
  tft_.setCursor(10, 118);
  tft_.print(winner->balance);
  tft_.setTextSize(2);
  tft_.setCursor(10, 170);
  tft_.print("winner");
}

void DisplayUi::render(const GameLogic &game, float batteryPercent) {
  drawStatusBar(game.state(), batteryPercent);
  clearMain();

  switch (game.state()) {
    case UiState::HOME:
      drawHome(game);
      break;
    case UiState::WAIT_CARD:
      drawWaitCard(game.context());
      break;
    case UiState::PROPERTY_UNOWNED:
      drawPropertyUnowned(game, game.context());
      break;
    case UiState::PROPERTY_OWNED:
      drawPropertyOwned(game, game.context());
      break;
    case UiState::EVENT:
      drawEvent(game.context());
      break;
    case UiState::AUCTION:
      drawAuction(game.context());
      break;
    case UiState::DEBT:
      drawDebt(game.context());
      break;
    case UiState::GO:
      drawGo();
      break;
    case UiState::JAIL:
      drawJail();
      break;
    case UiState::TRAIN:
      drawTrain();
      break;
    case UiState::WINNER:
      drawWinner(game, game.context());
      break;
  }
}

void DisplayUi::renderProgramming(const char *category, uint8_t itemId, const char *detail, bool armWrite, const char *message) {
  (void)category;
  (void)itemId;
  (void)detail;
  (void)armWrite;
  (void)message;

  clearMain();
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(8, 8);
  tft_.print("program mode");

  tft_.setTextSize(3);
  tft_.setCursor(8, 42);
  tft_.print(category);
  tft_.setCursor(220, 42);
  tft_.print(itemId);

  tft_.setTextSize(2);
  tft_.setCursor(8, 88);
  tft_.print(detail);

  tft_.setTextSize(1);
  tft_.setCursor(8, 186);
  tft_.print("X:prev  M:write  Y:next");
  tft_.setCursor(8, 202);
  tft_.print("X long:prev cat  Y long:next cat");
  tft_.setCursor(8, 218);
  tft_.print("hold X+CHECK 5s: exit");

  if (armWrite) {
    tft_.setTextSize(2);
    tft_.setCursor(8, 150);
    tft_.print("tap card...");
  }

  if (message && message[0] != '\0') {
    tft_.setTextSize(2);
    tft_.setCursor(8, 124);
    tft_.print(message);
  }
}

void DisplayUi::renderLobby(uint8_t registeredCount, uint8_t requiredCount, const bool activePlayers[GAME_MAX_PLAYERS], bool fundingStage, const char *message) {
  clearMain();
  tft_.fillRect(0, 0, SCREEN_W, 20, SOFT);
  tft_.drawFastHLine(0, 20, SCREEN_W, FG);
  tft_.setTextColor(FG);
  tft_.setTextSize(1);
  tft_.setCursor(6, 6);
  tft_.print("setup");

  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  tft_.setTextSize(2);
  tft_.setCursor(12, 36);
  tft_.print(fundingStage ? "fund players" : "register players");

  tft_.setTextSize(3);
  tft_.setCursor(12, 72);
  tft_.print(registeredCount);
  tft_.print('/');
  tft_.print(requiredCount);

  int y = 120;
  tft_.setTextSize(2);
  for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
    if (!activePlayers[i]) continue;
    drawTokenGlyph(tft_, 12, y + 4, i + 1);
    tft_.setCursor(34, y);
    tft_.print(i + 1);
    y += 24;
  }

  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  if (fundingStage) {
    tft_.print("tap each player card to add money");
  } else {
    tft_.print("tap player cards, then CHECK");
  }
  tft_.setCursor(8, 228);
  tft_.print("X:clear   M:menu   Y:start");

  if (message && message[0] != '\0') {
    tft_.setCursor(150, 228);
    tft_.print(message);
  }
}

void DisplayUi::renderActionMenu(uint8_t selected) {
  clearMain();
  tft_.fillRect(0, 0, SCREEN_W, 20, SOFT);
  tft_.drawFastHLine(0, 20, SCREEN_W, FG);
  tft_.setTextColor(FG);
  tft_.setTextSize(1);
  tft_.setCursor(6, 6);
  tft_.print("menu");

  tft_.drawRect(6, 28, SCREEN_W - 12, 176, FG);
  const char *labels[3] = {"GO +200", "JAIL -100", "TRAIN -100"};
  for (uint8_t i = 0; i < 3; i++) {
    const int y = 44 + i * 52;
    if (i == selected) {
      tft_.fillRect(12, y - 2, SCREEN_W - 24, 40, SOFT);
    }
    tft_.drawRect(12, y - 2, SCREEN_W - 24, 40, FG);
    tft_.setTextSize(2);
    tft_.setCursor(24, y + 8);
    if (i == 0) tft_.print("# ");
    if (i == 1) tft_.print("[] ");
    if (i == 2) tft_.print("= ");
    tft_.print(labels[i]);
  }

  tft_.setTextSize(1);
  tft_.setCursor(8, 212);
  tft_.print("X:close  M:next  Y:select");
}
