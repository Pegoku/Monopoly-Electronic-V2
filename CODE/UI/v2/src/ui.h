#pragma once
#include <Arduino.h>
#include "config.h"
#include "game_logic.h"

// =============================================================================
// UI ENTRY POINTS  (called from main loop)
// =============================================================================
void ui_init();
void ui_update();           // Handles drawing + input every frame

// =============================================================================
// WIDGET HELPERS  (used internally but exposed for extensibility)
// =============================================================================

// Simple rounded-rect button; returns true if tapped this frame
bool ui_drawButton(int16_t x, int16_t y, int16_t w, int16_t h,
                   const char* label, uint16_t bg, uint16_t fg,
                   bool highlight = false);

// Check if current frame's touch is inside rect
bool ui_tapped(int16_t x, int16_t y, int16_t w, int16_t h);

// Draw header bar at top
void ui_drawHeader(const char* title, uint16_t accent = COL_PRIMARY);

// Draw player status (small bar)
void ui_drawPlayerMini(int16_t x, int16_t y, uint8_t idx);

// Draw dice face at (cx,cy) with half-size s
void ui_drawDie(int16_t cx, int16_t cy, int16_t s, uint8_t value, uint16_t col);

// On-screen keyboard (modal, blocks until done)
bool ui_keyboard(char* buf, uint8_t maxLen, const char* prompt);
