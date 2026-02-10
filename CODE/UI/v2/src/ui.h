#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "game_logic.h"

// =============================================================================
// UI ENTRY POINTS  (called from main loop)
// =============================================================================
void ui_init();
void ui_update();           // Check game state, rebuild screens as needed

// =============================================================================
// ON-SCREEN KEYBOARD (blocking – runs its own LVGL loop)
// =============================================================================
bool ui_keyboard(char* buf, uint8_t maxLen, const char* prompt);
