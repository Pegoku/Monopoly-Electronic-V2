#include <Arduino.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "battery_manager.h"
#include "card_manager.h"
#include "config.h"
#include "display_ui.h"
#include "game_logic.h"
#include "nfc_manager.h"
#include "sound_manager.h"

namespace {
NfcManager nfc;
CardManager *cards = nullptr;
GameLogic game;
DisplayUi ui;
BatteryManager battery;
SoundManager sound;

uint32_t lastRenderMs = 0;
UiState lastState = UiState::HOME;

struct ButtonInput {
  int pin = -1;
  bool stablePressed = false;
  bool rawPressed = false;
  uint32_t lastDebounceMs = 0;
  uint32_t pressStartMs = 0;
};

ButtonInput btn1;
ButtonInput btn2;
ButtonInput btn3;

enum class ButtonPress : uint8_t { None, Short, Long };
enum class ProgramCategory : uint8_t { Player, Property, Event };
enum class AppMode : uint8_t { LobbyRegister, Running };

constexpr uint32_t LONG_PRESS_MS = 700;
constexpr uint32_t PROGRAM_MODE_HOLD_MS = 5000;

bool programmingMode = false;
bool programArmed = false;
ProgramCategory programCategory = ProgramCategory::Player;
uint8_t programIndex = 0;
char programDetail[40] = {0};
char programMessage[40] = {0};
uint32_t programMessageUntilMs = 0;
uint32_t comboHoldStartMs = 0;
bool comboLatch = false;
bool uiDirty = true;
int8_t lastWaitAnimPhase = -1;

AppMode appMode = AppMode::LobbyRegister;
bool activeLobbyPlayers[GAME_MAX_PLAYERS] = {false};
bool fundedLobbyPlayers[GAME_MAX_PLAYERS] = {false};
uint8_t registeredCount = 0;
uint8_t requiredPlayerCount = 0;
char lobbyMessage[40] = {0};

bool actionMenuOpen = false;
uint8_t actionMenuIndex = 0;

void logLine(const char *text) {
  Serial.println(text);
  Serial0.println(text);
}

void logf(const char *fmt, ...) {
  char buf[160];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
  Serial0.println(buf);
}

void setLobbyMessage(const char *msg) {
  strncpy(lobbyMessage, msg, sizeof(lobbyMessage) - 1);
  lobbyMessage[sizeof(lobbyMessage) - 1] = '\0';
  uiDirty = true;
}

void clearLobbyData() {
  for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
    activeLobbyPlayers[i] = false;
    fundedLobbyPlayers[i] = false;
  }
  registeredCount = 0;
  requiredPlayerCount = 0;
  setLobbyMessage("tap player cards");
}

const char *stateName(UiState state) {
  switch (state) {
    case UiState::HOME:
      return "HOME";
    case UiState::WAIT_CARD:
      return "WAIT_CARD";
    case UiState::PROPERTY_UNOWNED:
      return "PROPERTY_UNOWNED";
    case UiState::PROPERTY_OWNED:
      return "PROPERTY_OWNED";
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
  return "UNKNOWN";
}

void logStateTransition(const char *reason) {
  const UiState now = game.state();
  if (now == lastState) return;
  logf("[STATE] %s -> %s (%s)", stateName(lastState), stateName(now), reason);
  lastState = now;
}

void printUid(const CardTap &tap) {
  Serial.print("UID=");
  Serial0.print("UID=");
  for (uint8_t i = 0; i < tap.uidLen; i++) {
    if (tap.uid[i] < 0x10) {
      Serial.print('0');
      Serial0.print('0');
    }
    Serial.print(tap.uid[i], HEX);
    Serial0.print(tap.uid[i], HEX);
  }
}

ButtonPress pollButton(ButtonInput &btn) {
  const bool raw = digitalRead(btn.pin) == LOW;
  const uint32_t now = millis();

  if (raw != btn.rawPressed) {
    btn.rawPressed = raw;
    btn.lastDebounceMs = now;
  }

  if ((now - btn.lastDebounceMs) < BTN_DEBOUNCE_MS) {
    return ButtonPress::None;
  }

  if (btn.stablePressed != btn.rawPressed) {
    btn.stablePressed = btn.rawPressed;
    if (btn.stablePressed) {
      btn.pressStartMs = now;
      return ButtonPress::None;
    }
    const uint32_t heldMs = now - btn.pressStartMs;
    return heldMs >= LONG_PRESS_MS ? ButtonPress::Long : ButtonPress::Short;
  }

  return ButtonPress::None;
}

const char *programCategoryName(ProgramCategory cat) {
  switch (cat) {
    case ProgramCategory::Player:
      return "PLAYER";
    case ProgramCategory::Property:
      return "PLACE";
    case ProgramCategory::Event:
      return "SPECIAL";
  }
  return "?";
}

void logProgramSelection() {
  logf("[PROG] category=%s number=%u", programCategoryName(programCategory), programIndex + 1);
}

uint8_t maxIndexForCategory(ProgramCategory cat) {
  if (cat == ProgramCategory::Player) return GAME_MAX_PLAYERS - 1;
  if (cat == ProgramCategory::Property) return 27;
  return 5;
}

void setProgramMessage(const char *msg, uint32_t holdMs = 1000) {
  strncpy(programMessage, msg, sizeof(programMessage) - 1);
  programMessage[sizeof(programMessage) - 1] = '\0';
  programMessageUntilMs = millis() + holdMs;
  uiDirty = true;
}

void updateProgramDetail() {
  if (programCategory == ProgramCategory::Player) {
    snprintf(programDetail, sizeof(programDetail), "ID %u BAL %u", programIndex + 1, STARTING_MONEY);
    logProgramSelection();
    uiDirty = true;
    return;
  }
  if (programCategory == ProgramCategory::Property) {
    const uint8_t id = programIndex + 1;
    const uint16_t price = 80 + static_cast<uint16_t>(id * 12.5f + 0.5f);
    snprintf(programDetail, sizeof(programDetail), "#%u COST %u", id, price);
    logProgramSelection();
    uiDirty = true;
    return;
  }

  static const int16_t values[] = {200, -150, 0, 1, 100, -200};
  static const char *types[] = {"MONEY+", "MONEY-", "JAIL", "RENT+", "MONEY+", "MONEY-"};
  snprintf(programDetail, sizeof(programDetail), "#%u %s %d", programIndex + 1, types[programIndex], values[programIndex]);
  logProgramSelection();
  uiDirty = true;
}

void switchCategory(int dir) {
  int next = static_cast<int>(programCategory) + dir;
  if (next < 0) next = 2;
  if (next > 2) next = 0;
  programCategory = static_cast<ProgramCategory>(next);
  const uint8_t max = maxIndexForCategory(programCategory);
  if (programIndex > max) programIndex = max;
  updateProgramDetail();
}

void moveItem(int dir) {
  const int max = maxIndexForCategory(programCategory);
  int next = static_cast<int>(programIndex) + dir;
  if (next < 0) next = max;
  if (next > max) next = 0;
  programIndex = static_cast<uint8_t>(next);
  updateProgramDetail();
}

void handleButtons() {
  const ButtonPress b1 = pollButton(btn1);
  const ButtonPress b2 = pollButton(btn2);
  const ButtonPress b3 = pollButton(btn3);

  if (programmingMode) {
    if (b1 == ButtonPress::Short) {
      moveItem(-1);
      logLine("[PROG] item back");
      sound.beepTick();
    } else if (b1 == ButtonPress::Long) {
      switchCategory(-1);
      logLine("[PROG] prev category");
      sound.beepTick();
    }

    if (b3 == ButtonPress::Short) {
      moveItem(1);
      logLine("[PROG] item next");
      sound.beepTick();
    } else if (b3 == ButtonPress::Long) {
      switchCategory(1);
      logLine("[PROG] next category");
      sound.beepTick();
    }

    if (b2 == ButtonPress::Short) {
      programArmed = true;
      setProgramMessage("TAP CARD", 2000);
      logLine("[PROG] arm write, waiting for card");
      sound.beepTick();
      uiDirty = true;
    }
    return;
  }

  if (appMode == AppMode::LobbyRegister) {
    if (b1 == ButtonPress::Short) {
      clearLobbyData();
      logLine("[LOBBY] cleared");
      sound.beepTick();
    }
    if (b3 == ButtonPress::Short) {
      if (registeredCount >= 2) {
        requiredPlayerCount = registeredCount;
        for (uint8_t i = 0; i < GAME_MAX_PLAYERS; i++) {
          if (!activeLobbyPlayers[i]) continue;
          fundedLobbyPlayers[i] = true;
          game.primePlayer(i + 1, STARTING_MONEY);
        }
        appMode = AppMode::Running;
        setLobbyMessage("players funded, game start");
        logf("[LOBBY] game start with %u players", requiredPlayerCount);
        sound.beepOk();
      } else {
        setLobbyMessage("need at least 2 players");
        sound.beepError();
      }
    }
    return;
  }

  // Physical mapping:
  // BTN1 = X (cancel/back), BTN2 = M (mode/special), BTN3 = CHECK (confirm)
  if (actionMenuOpen) {
    if (b2 == ButtonPress::Short) {
      actionMenuIndex = (actionMenuIndex + 1) % 3;
      uiDirty = true;
      sound.beepTick();
      logf("[MENU] next option=%u", actionMenuIndex);
    }
    if (b1 == ButtonPress::Short) {
      actionMenuOpen = false;
      uiDirty = true;
      sound.beepTick();
      logLine("[MENU] close");
    }
    if (b3 == ButtonPress::Short) {
      actionMenuOpen = false;
      game.triggerMenuAction(actionMenuIndex);
      uiDirty = true;
      sound.beepOk();
      logf("[MENU] selected option=%u", actionMenuIndex);
      logStateTransition("MENU_SELECT");
    }
    return;
  }

  if (b1 == ButtonPress::Short) {
    logLine("[BTN] X pressed");
    game.onBtn2();
    sound.beepTick();
    logStateTransition("X");
  }
  if (b2 == ButtonPress::Short) {
    if (game.state() == UiState::HOME) {
      actionMenuOpen = true;
      actionMenuIndex = 0;
      uiDirty = true;
      sound.beepTick();
      logLine("[MENU] open");
    } else {
      logLine("[BTN] M pressed");
      game.onBtn3();
      sound.beepTick();
      logStateTransition("M");
    }
  }
  if (b3 == ButtonPress::Short) {
    logLine("[BTN] CHECK pressed");
    game.onBtn1();
    sound.beepTick();
    logStateTransition("CHECK");
  }
}

void handleProgramCombo() {
  const bool hold = (digitalRead(PIN_BTN1) == LOW) && (digitalRead(PIN_BTN3) == LOW);
  const uint32_t now = millis();

  if (!hold) {
    comboHoldStartMs = 0;
    comboLatch = false;
    return;
  }

  if (comboLatch) {
    return;
  }
  if (comboHoldStartMs == 0) {
    comboHoldStartMs = now;
    return;
  }
  if (now - comboHoldStartMs < PROGRAM_MODE_HOLD_MS) {
    return;
  }

  comboLatch = true;
  programmingMode = !programmingMode;
  programArmed = false;
  if (programmingMode) {
    programCategory = ProgramCategory::Player;
    programIndex = 0;
    updateProgramDetail();
    setProgramMessage("PROGRAM MODE", 1200);
    logLine("[PROG] enabled");
  } else {
    setProgramMessage("PROGRAM OFF", 1200);
    logLine("[PROG] disabled");
  }
  sound.beepOk();
  uiDirty = true;
}

void handleCardTap() {
  CardTap tap{};
  if (!nfc.poll(tap)) return;

  if (appMode == AppMode::LobbyRegister) {
    PlayerCardData player{};
    if (!cards->readPlayer(tap, player)) {
      setLobbyMessage("tap a PLAYER card");
      sound.beepError();
      return;
    }
    if (player.playerId < 1 || player.playerId > GAME_MAX_PLAYERS) {
      setLobbyMessage("player id out of range");
      sound.beepError();
      return;
    }
    const uint8_t idx = player.playerId - 1;
    if (!activeLobbyPlayers[idx]) {
      activeLobbyPlayers[idx] = true;
      registeredCount++;
      logf("[LOBBY] registered player=%u total=%u", player.playerId, registeredCount);
    }
    setLobbyMessage("player registered");
    uiDirty = true;
    sound.beepOk();
    return;
  }

  if (programmingMode) {
    if (!programArmed) {
      logLine("[PROG] card ignored (press BTN2 to arm write)");
      sound.beepError();
      return;
    }

    bool ok = false;
    if (programCategory == ProgramCategory::Player) {
      PlayerCardData player{};
      player.playerId = programIndex + 1;
      player.balance = STARTING_MONEY;
      player.jailed = 0;
      player.bankrupt = 0;
      ok = cards->writePlayer(tap, player);
      logf("[PROG] write player id=%u result=%s", player.playerId, ok ? "ok" : "fail");
    } else if (programCategory == ProgramCategory::Property) {
      const uint8_t id = programIndex + 1;
      PropertyCardData property{};
      property.propertyId = id;
      property.ownerId = 0;
      property.level = 1;
      property.basePrice = 80 + static_cast<uint16_t>(id * 12.5f + 0.5f);
      ok = cards->writeProperty(tap, property);
      logf("[PROG] write place id=%u price=%u result=%s", id, property.basePrice, ok ? "ok" : "fail");
    } else {
      EventCardData event{};
      event.eventId = programIndex + 1;
      if (programIndex == 0) {
        event.type = EventType::MONEY;
        event.value = 200;
      } else if (programIndex == 1) {
        event.type = EventType::MONEY;
        event.value = -150;
      } else if (programIndex == 2) {
        event.type = EventType::JAIL;
        event.value = 0;
      } else if (programIndex == 3) {
        event.type = EventType::RENT_BOOST;
        event.value = 1;
      } else if (programIndex == 4) {
        event.type = EventType::MONEY;
        event.value = 100;
      } else {
        event.type = EventType::MONEY;
        event.value = -200;
      }
      ok = cards->writeEvent(tap, event);
      logf("[PROG] write special id=%u result=%s", event.eventId, ok ? "ok" : "fail");
    }

    printUid(tap);
    Serial.println();
    Serial0.println();
    if (ok) {
      setProgramMessage("WRITE OK", 1200);
      sound.beepOk();
    } else {
      setProgramMessage("WRITE FAIL", 1200);
      sound.beepError();
    }
    programArmed = false;
    return;
  }

  NfcCardType type = cards->detectType(tap);
  if (type == NfcCardType::PLAYER) {
    PlayerCardData player{};
    if (cards->readPlayer(tap, player)) {
      if (appMode == AppMode::Running) {
        if (player.playerId < 1 || player.playerId > GAME_MAX_PLAYERS || !activeLobbyPlayers[player.playerId - 1]) {
          setLobbyMessage("player not in this game");
          uiDirty = true;
          sound.beepError();
          return;
        }
      }
      logf("[NFC] player card scanned id=%u balance=%ld jailed=%u bankrupt=%u", player.playerId, (long)player.balance, player.jailed, player.bankrupt);
      printUid(tap);
      Serial.println();
      Serial0.println();
    } else {
      Serial.print("[NFC] player card scanned (read failed) ");
      Serial0.print("[NFC] player card scanned (read failed) ");
      printUid(tap);
      Serial.println();
      Serial0.println();
    }
    game.onPlayerCard(tap, *cards);
    sound.beepOk();
    logStateTransition("PLAYER_CARD");
    return;
  }
  if (type == NfcCardType::PROPERTY) {
    PropertyCardData property{};
    if (cards->readProperty(tap, property)) {
      logf("[NFC] property card scanned id=%u owner=%u level=%u price=%u", property.propertyId, property.ownerId, property.level, property.basePrice);
      printUid(tap);
      Serial.println();
      Serial0.println();
    } else {
      Serial.print("[NFC] property card scanned (read failed) ");
      Serial0.print("[NFC] property card scanned (read failed) ");
      printUid(tap);
      Serial.println();
      Serial0.println();
    }
    game.onPropertyCard(tap, *cards);
    sound.beepOk();
    logStateTransition("PROPERTY_CARD");
    return;
  }
  if (type == NfcCardType::EVENT) {
    EventCardData event{};
    if (cards->readEvent(tap, event)) {
      logf("[NFC] event card scanned id=%u type=%u value=%d", event.eventId, static_cast<uint8_t>(event.type), event.value);
      printUid(tap);
      Serial.println();
      Serial0.println();
    } else {
      Serial.print("[NFC] event card scanned (read failed) ");
      Serial0.print("[NFC] event card scanned (read failed) ");
      printUid(tap);
      Serial.println();
      Serial0.println();
    }
    game.onEventCard(tap, *cards);
    sound.beepOk();
    logStateTransition("EVENT_CARD");
    return;
  }

  Serial.print("[NFC] unknown card scanned ");
  Serial0.print("[NFC] unknown card scanned ");
  printUid(tap);
  Serial.println();
  Serial0.println();
  sound.beepError();
}

void refreshUi(bool force = false) {
  const uint32_t now = millis();
  if (programMessageUntilMs > 0 && now >= programMessageUntilMs) {
    programMessage[0] = '\0';
    programMessageUntilMs = 0;
    uiDirty = true;
  }

  bool animDirty = false;
  const bool waitAnim = (!programmingMode && appMode == AppMode::Running && !actionMenuOpen && game.state() == UiState::WAIT_CARD);
  if (waitAnim) {
    const int8_t phase = static_cast<int8_t>((now / 120) % 2);
    if (phase != lastWaitAnimPhase) {
      lastWaitAnimPhase = phase;
      animDirty = true;
    }
  } else {
    lastWaitAnimPhase = -1;
  }

  const bool gameDirty = (!programmingMode && appMode == AppMode::Running && !actionMenuOpen && game.isDirty());
  const bool needsRender = force || uiDirty || gameDirty || animDirty;

  if (!needsRender) {
    return;
  }

  if (programmingMode) {
    ui.renderProgramming(programCategoryName(programCategory), programIndex + 1, programDetail, programArmed, programMessage);
  } else if (appMode != AppMode::Running) {
    ui.renderLobby(registeredCount, registeredCount, activeLobbyPlayers, false, lobbyMessage);
  } else if (actionMenuOpen) {
    ui.renderActionMenu(actionMenuIndex);
  } else {
    ui.render(game, battery.readPercent());
    game.clearDirty();
  }

  uiDirty = false;
  lastRenderMs = now;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(100);
  logLine("[BOOT] booting...");
  logLine("[BOOT] RAM_MODE: gameplay money/property kept in memory (no gameplay card writes)");

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_BTN3, INPUT_PULLUP);

  btn1.pin = PIN_BTN1;
  btn2.pin = PIN_BTN2;
  btn3.pin = PIN_BTN3;

  battery.begin();
  logLine("[BOOT] battery manager ready");
  sound.begin();
  logLine("[BOOT] sound manager ready");
  ui.begin();
  logLine("[BOOT] display ready");

  const bool nfcOk = nfc.begin();
  logf("[BOOT] nfc init: %s", nfcOk ? "ok" : "failed");
  cards = new CardManager(nfc.driver());

  game.begin();
  clearLobbyData();
  appMode = AppMode::LobbyRegister;
  updateProgramDetail();
  lastState = game.state();
  logf("[BOOT] game state: %s", stateName(lastState));
  if (!nfcOk) {
    game.onBtn2();
    logStateTransition("NFC_FAIL_FALLBACK");
  }

  refreshUi(true);
  logLine("[BOOT] running");
}

void loop() {
  handleProgramCombo();
  handleButtons();
  handleCardTap();
  if (!programmingMode && appMode == AppMode::Running && !actionMenuOpen) {
    game.onTick();
    logStateTransition("TICK");
  }
  refreshUi();
}
