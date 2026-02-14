#include "display_ui.h"

#include <SPI.h>

namespace {
constexpr uint16_t BG = ST77XX_WHITE;
constexpr uint16_t FG = ST77XX_BLACK;
constexpr uint16_t ACCENT = ST77XX_WHITE;

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
    tft.fillTriangle(x, y + 4, x + 10, y, x + 10, y + 8, FG);
    return;
  }
  if (kind == 1) {
    tft.fillRect(x + 2, y, 8, 8, FG);
    tft.fillRect(x + 10, y + 2, 3, 4, FG);
    return;
  }
  if (kind == 2) {
    tft.drawCircle(x + 5, y + 4, 4, FG);
    tft.drawFastHLine(x + 9, y + 4, 4, FG);
    return;
  }
  tft.fillTriangle(x, y + 2, x + 10, y + 2, x + 5, y + 8, FG);
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
  (void)state;
  (void)batteryPercent;
}

void DisplayUi::drawHome(const GameLogic &game) {
  const PlayerState *players = game.players();

  tft_.setTextColor(FG);
  tft_.setTextSize(3);
  int y = 20;
  bool any = false;
  for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
    if (!players[i].active || players[i].bankrupt) continue;
    any = true;
    drawTokenGlyph(tft_, 8, y + 6, players[i].id);
    tft_.setCursor(36, y);
    tft_.print(players[i].balance);
    y += 34;
    if (y > 200) break;
  }

  if (!any) {
    tft_.setTextSize(2);
    tft_.setCursor(12, 92);
    tft_.print("tap player card");
  }

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

  if (ctx.noCancel) {
    tft_.setTextSize(1);
    tft_.setCursor(8, 228);
    tft_.print("no cancel");
  }
}

void DisplayUi::drawPropertyUnowned(const GameLogic &game, const ActionContext &ctx) {
  const PropertyState *prop = &game.properties()[ctx.propertyId - 1];
  tft_.setTextColor(FG);
  tft_.setTextSize(3);
  tft_.setCursor(20, 62);
  tft_.print(prop->id);
  tft_.setCursor(20, 120);
  tft_.print(prop->basePrice);
  tft_.setTextSize(2);
  tft_.setCursor(248, 62);
  tft_.print("house");
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  tft_.print("BTN1 buy  BTN3 auction  BTN2 back");
}

void DisplayUi::drawPropertyOwned(const GameLogic &game, const ActionContext &ctx) {
  const PropertyState *prop = &game.properties()[ctx.propertyId - 1];
  const int rent = prop->baseRent * prop->level;
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 66);
  tft_.print(iconByPlayerId(prop->ownerId));
  tft_.setCursor(106, 66);
  tft_.print(rent);
  tft_.setCursor(10, 110);
  tft_.print(prop->id);
  tft_.setCursor(106, 110);
  tft_.print(prop->level);
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  tft_.print("BTN1 continue");
}

void DisplayUi::drawEvent(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(12, 80);
  tft_.print("event");
  tft_.setTextSize(3);
  tft_.setCursor(12, 118);
  tft_.print(ctx.eventId);
}

void DisplayUi::drawAuction(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 64);
  tft_.print("auction");
  tft_.setCursor(180, 64);
  tft_.print(ctx.auctionSecondsLeft);
  tft_.setTextSize(3);
  tft_.setCursor(10, 114);
  tft_.print(ctx.auctionBid);
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  if (ctx.auctionAwaitWinner) {
    tft_.print("winner tap bank card");
  } else {
    tft_.print("BTN1 +20");
  }
}

void DisplayUi::drawDebt(const ActionContext &ctx) {
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 70);
  tft_.print("debt");
  tft_.setTextSize(3);
  tft_.setCursor(10, 120);
  tft_.print('-');
  tft_.print(ctx.debtAmount);
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  tft_.print("tap debtor property cards");
}

void DisplayUi::drawGo() {
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 90);
  tft_.print("go");
  tft_.setTextSize(3);
  tft_.setCursor(10, 130);
  tft_.print("+200");
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  tft_.print("tap player bank card");
}

void DisplayUi::drawJail() {
  tft_.setTextColor(FG);
  tft_.setTextSize(2);
  tft_.setCursor(10, 90);
  tft_.print("jail");
  tft_.setTextSize(3);
  tft_.setCursor(10, 130);
  tft_.print("100");
  tft_.setTextSize(1);
  tft_.setCursor(8, 220);
  tft_.print("tap player bank card");
}

void DisplayUi::drawWinner(const GameLogic &game, const ActionContext &ctx) {
  const PlayerState *winner = &game.players()[ctx.debtorId - 1];
  tft_.setTextColor(FG);
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
  tft_.print("B2:write  B1:prev  B3:next");
  tft_.setCursor(8, 202);
  tft_.print("B1 long:prev cat  B3 long:next cat");
  tft_.setCursor(8, 218);
  tft_.print("hold B1+B3 5s: exit");

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
