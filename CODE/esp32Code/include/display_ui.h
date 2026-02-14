#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "config.h"
#include "game_logic.h"

class DisplayUi {
 public:
  bool begin();
  void render(const GameLogic &game, float batteryPercent);
  void renderProgramming(const char *category, uint8_t itemId, const char *detail, bool armWrite, const char *message);

 private:
  void drawHome(const GameLogic &game);
  void drawWaitCard(const ActionContext &ctx);
  void drawPropertyUnowned(const GameLogic &game, const ActionContext &ctx);
  void drawPropertyOwned(const GameLogic &game, const ActionContext &ctx);
  void drawEvent(const ActionContext &ctx);
  void drawAuction(const ActionContext &ctx);
  void drawDebt(const ActionContext &ctx);
  void drawGo();
  void drawJail();
  void drawWinner(const GameLogic &game, const ActionContext &ctx);
  void drawStatusBar(UiState state, float batteryPercent);
  void clearMain();

  Adafruit_ST7789 tft_{PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCLK, PIN_TFT_RST};
};
