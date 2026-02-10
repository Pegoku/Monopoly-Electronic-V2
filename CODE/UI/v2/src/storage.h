#pragma once
#include <Arduino.h>
#include "game_logic.h"

// Save / load game state to ESP32 NVS (Preferences)
bool storage_saveGame();
bool storage_loadGame();
bool storage_hasSavedGame();
void storage_clearSave();

// Settings persistence
bool storage_saveSettings(const GameSettings& s);
bool storage_loadSettings(GameSettings& s);
