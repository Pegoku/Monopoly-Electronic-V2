#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "config.h"

// =============================================================================
// POWER / BATTERY (BQ25895 charger)
// =============================================================================
struct BatteryInfo {
    bool     present      = false;   // true if BQ25895 responded
    bool     powerGood    = false;   // VIN present (PG_STAT)
    bool     charging     = false;   // CHG_STAT not "not charging"
    uint8_t  percent      = 0;       // 0-100, derived from VBAT reading
    float    voltage      = 0.0f;    // battery voltage (V)
    uint32_t lastUpdateMs = 0;       // millis() timestamp of last poll
};

void        hw_initPower();          // Configure charger (1A) + start ADC
void        hw_updatePower();        // Poll VBAT/CHG status (lightweight)
BatteryInfo hw_getBatteryInfo();

// =============================================================================
// DISPLAY (TFT_eSPI used as LVGL backend)
// =============================================================================
extern TFT_eSPI tft;

void     hw_initDisplay();
void     hw_backlight(bool on);

// =============================================================================
// LVGL DRIVERS  (display flush + touch + keypad)
// =============================================================================
void     hw_lvgl_init();           // Call AFTER hw_initDisplay + hw_initTouch + hw_initButtons

// =============================================================================
// TOUCH
// =============================================================================
struct TouchPoint {
    int16_t x = -1, y = -1;
    bool    pressed = false;
};

void       hw_initTouch();
TouchPoint hw_getTouch();
bool       hw_touchInRect(int16_t tx, int16_t ty,
                          int16_t rx, int16_t ry, int16_t rw, int16_t rh);

// =============================================================================
// BUTTONS
// =============================================================================
enum BtnId : uint8_t { BTN_NONE = 0, BTN_LEFT, BTN_CENTER, BTN_RIGHT };

void  hw_initButtons();
BtnId hw_readButtons();                         // debounced, single-press
bool  hw_isBtnHeld(BtnId b, uint32_t ms);      // true while held >= ms

// =============================================================================
// AUDIO  (non-blocking melody system)
// =============================================================================
struct Note { uint16_t freq; uint16_t durationMs; };

void hw_initAudio();
void hw_updateAudio();                          // call every loop iteration
void hw_setVolume(uint8_t vol);                 // 0-5 (0 = mute)
void hw_playMelody(const Note* melody, uint8_t len);

// Pre-built melodies
void hw_playJingle();
void hw_playDiceRoll();
void hw_playCashIn();
void hw_playCashOut();
void hw_playCardDraw();
void hw_playError();
void hw_playSuccess();
void hw_playJail();
