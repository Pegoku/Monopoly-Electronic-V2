// =============================================================================
// MONOPOLY ELECTRONIC V2 — Main entry point
// =============================================================================
#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "nfc_handler.h"
#include "game_logic.h"
#include "storage.h"
#include "ui.h"

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("\n=== Monopoly Electronic V2 ==="));

    // Seed RNG
    randomSeed(analogRead(0) ^ (millis() << 8));

    // Hardware init
    hw_initDisplay();
    hw_initTouch();
    hw_initButtons();
    hw_initAudio();

    // NFC (non-blocking — game works without it)
    if (nfc_init()) {
        Serial.println(F("[INIT] NFC ready"));
    } else {
        Serial.println(F("[INIT] NFC not detected — continuing without NFC"));
    }

    // Load saved settings (if any)
    storage_loadSettings(G.settings);
    hw_setVolume(G.settings.volume);

    // Init game state
    game_init();
    G.phase = PHASE_SPLASH;

    // Init UI
    ui_init();

    // Startup jingle
    hw_playJingle();

    Serial.println(F("[INIT] Ready"));
}

void loop() {
    hw_updateAudio();       // advance non-blocking melodies
    ui_update();            // draw + handle input
    delay(16);              // ~60 fps cap
}