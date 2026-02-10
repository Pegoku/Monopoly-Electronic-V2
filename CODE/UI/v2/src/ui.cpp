#include "ui.h"
#include "hardware.h"
#include "nfc_handler.h"
#include "storage.h"
#include "config.h"
#include <TFT_eSPI.h>

// =============================================================================
// LOCAL STATE
// =============================================================================
static TouchPoint _tp;
static BtnId      _btn;
static uint32_t   _frameTime;
static bool       _needRedraw = true;

// Dice animation
static uint32_t _diceAnimStart = 0;
static uint8_t  _animD1 = 1, _animD2 = 1;

// Splash timer
static uint32_t _splashStart = 0;

// Menu selection
static int8_t _menuSel = 0;

// Setup
static uint8_t _setupCount = 2;
static uint8_t _setupRegistered = 0;
static bool    _setupNfcScanning = false;

// Quick-menu scroll offset
static int8_t _qmSel = 0;

// Programming mode
static uint8_t _progMode = 0;   // 0=select, 1=player, 2=property, 3=event
static uint8_t _progStep = 0;

// Settings scroll
static int8_t  _settSel = 0;

// Trade UI state
static int8_t  _tradeSel = 0;
static int8_t  _tradePlayerSel = -1;
static int8_t  _tradePropScroll = 0;

// Property list for owned-property browsing
static int8_t  _propListScroll = 0;
static int8_t  _propListSel = -1;

// Screen-local flag to prevent re-triggering touch
static bool _touchConsumed = false;

// =============================================================================
// FONT SHORTHAND  (TFT_eSPI built-in: 1=8px,2=16px,4=26px)
// =============================================================================
#define F_SM  1
#define F_MD  2
#define F_LG  4

// =============================================================================
//  WIDGET HELPERS
// =============================================================================
bool ui_tapped(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (_touchConsumed || !_tp.pressed) return false;
    return hw_touchInRect(_tp.x, _tp.y, x, y, w, h);
}

bool ui_drawButton(int16_t x, int16_t y, int16_t w, int16_t h,
                   const char* label, uint16_t bg, uint16_t fg, bool highlight) {
    uint16_t border = highlight ? COL_ACCENT : COL_TEXT_DIM;
    tft.fillRoundRect(x, y, w, h, 6, bg);
    tft.drawRoundRect(x, y, w, h, 6, border);
    tft.setTextColor(fg, bg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(label, x + w / 2, y + h / 2, F_MD);
    tft.setTextDatum(TL_DATUM);

    bool tap = ui_tapped(x, y, w, h);
    if (tap) _touchConsumed = true;
    return tap;
}

void ui_drawHeader(const char* title, uint16_t accent) {
    tft.fillRect(0, 0, SCREEN_W, 28, accent);
    tft.setTextColor(COL_TEXT, accent);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, SCREEN_W / 2, 14, F_MD);
    tft.setTextDatum(TL_DATUM);
}

void ui_drawPlayerMini(int16_t x, int16_t y, uint8_t idx) {
    if (idx >= G.numPlayers) return;
    const Player& p = G.players[idx];
    uint16_t bg = p.alive ? COL_BG_DARK : RGB565(60, 20, 20);
    tft.fillRoundRect(x, y, 72, 22, 3, bg);

    // Colour dot
    tft.fillCircle(x + 8, y + 11, 5, p.colour);

    // Name (truncated)
    tft.setTextColor(COL_TEXT, bg);
    char buf[16];
    snprintf(buf, 6, "%s", p.name);
    tft.drawString(buf, x + 16, y + 3, F_SM);

    // Money
    snprintf(buf, 10, "$%ld", (long)p.money);
    tft.setTextColor(COL_ACCENT, bg);
    tft.drawString(buf, x + 16, y + 13, F_SM);
}

void ui_drawDie(int16_t cx, int16_t cy, int16_t s, uint8_t val, uint16_t col) {
    tft.fillRoundRect(cx - s, cy - s, s * 2, s * 2, 4, TFT_WHITE);
    tft.drawRoundRect(cx - s, cy - s, s * 2, s * 2, 4, col);
    int16_t d = s * 5 / 10;  // dot offset
    int16_t r = s / 5;       // dot radius
    uint16_t dc = TFT_BLACK;

    auto dot = [&](int16_t dx, int16_t dy) {
        tft.fillCircle(cx + dx, cy + dy, r, dc);
    };
    // Pip positions
    if (val == 1 || val == 3 || val == 5) dot(0, 0);
    if (val >= 2) { dot(-d, -d); dot(d, d); }
    if (val >= 4) { dot(d, -d); dot(-d, d); }
    if (val == 6)  { dot(-d, 0); dot(d, 0); }
}

// =============================================================================
//  ON-SCREEN KEYBOARD
// =============================================================================
static const char* _kbRows[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
static const uint8_t _kbRowLen[] = {10, 9, 7};

bool ui_keyboard(char* buf, uint8_t maxLen, const char* prompt) {
    uint8_t pos = strlen(buf);
    bool done = false;
    bool shift = true;

    while (!done) {
        tft.fillScreen(COL_BG);
        tft.setTextColor(COL_TEXT);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(prompt, SCREEN_W / 2, 14, F_MD);

        // Text field
        tft.fillRoundRect(10, 30, 300, 28, 4, COL_BG_DARK);
        tft.drawRoundRect(10, 30, 300, 28, 4, COL_ACCENT);
        tft.setTextColor(COL_TEXT, COL_BG_DARK);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(buf, 16, 44, F_MD);
        // cursor
        int16_t cw = tft.textWidth(buf, F_MD);
        tft.drawFastVLine(18 + cw, 34, 20, COL_ACCENT);

        // Keyboard rows
        int16_t ky = 68;
        for (int row = 0; row < 3; row++) {
            int16_t kx = (SCREEN_W - _kbRowLen[row] * 30) / 2;
            for (int c = 0; c < _kbRowLen[row]; c++) {
                char ch = _kbRows[row][c];
                if (!shift) ch = ch + 32; // lowercase
                char lbl[2] = {ch, 0};
                tft.fillRoundRect(kx, ky, 28, 28, 3, COL_BTN_BG);
                tft.drawRoundRect(kx, ky, 28, 28, 3, COL_TEXT_DIM);
                tft.setTextColor(COL_TEXT, COL_BTN_BG);
                tft.setTextDatum(MC_DATUM);
                tft.drawString(lbl, kx + 14, ky + 14, F_MD);
                kx += 30;
            }
            ky += 32;
        }

        // Bottom row: DEL, SPACE, DONE
        int16_t by = ky + 4;
        tft.fillRoundRect(10, by, 60, 30, 4, COL_DANGER);
        tft.setTextColor(COL_TEXT, COL_DANGER);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("DEL", 40, by + 15, F_MD);

        tft.fillRoundRect(80, by, 140, 30, 4, COL_BTN_BG);
        tft.setTextColor(COL_TEXT, COL_BTN_BG);
        tft.drawString("SPACE", 150, by + 15, F_MD);

        tft.fillRoundRect(230, by, 80, 30, 4, COL_BTN_ACTIVE);
        tft.setTextColor(COL_TEXT, COL_BTN_ACTIVE);
        tft.drawString("DONE", 270, by + 15, F_MD);

        tft.setTextDatum(TL_DATUM);

        // Wait for touch
        while (true) {
            hw_updateAudio();
            TouchPoint tp = hw_getTouch();
            BtnId b = hw_readButtons();

            if (b == BTN_LEFT) { // backspace
                if (pos > 0) { pos--; buf[pos] = 0; break; }
            }
            if (b == BTN_CENTER) { done = true; break; }

            if (!tp.pressed) continue;

            // Check key hits
            ky = 68;
            bool hit = false;
            for (int row = 0; row < 3 && !hit; row++) {
                int16_t kx = (SCREEN_W - _kbRowLen[row] * 30) / 2;
                for (int c = 0; c < _kbRowLen[row]; c++) {
                    if (hw_touchInRect(tp.x, tp.y, kx, ky, 28, 28)) {
                        if (pos < maxLen - 1) {
                            char ch = _kbRows[row][c];
                            if (!shift) ch += 32;
                            buf[pos++] = ch;
                            buf[pos] = 0;
                            shift = false;
                        }
                        hit = true; break;
                    }
                    kx += 30;
                }
                ky += 32;
            }
            if (hit) break;

            by = ky + 4;
            // DEL
            if (hw_touchInRect(tp.x, tp.y, 10, by, 60, 30)) {
                if (pos > 0) { pos--; buf[pos] = 0; }
                break;
            }
            // SPACE
            if (hw_touchInRect(tp.x, tp.y, 80, by, 140, 30)) {
                if (pos < maxLen - 1) { buf[pos++] = ' '; buf[pos] = 0; }
                break;
            }
            // DONE
            if (hw_touchInRect(tp.x, tp.y, 230, by, 80, 30)) {
                done = true; break;
            }
        }
    }
    return pos > 0;
}

// =============================================================================
//  SCREEN: SPLASH
// =============================================================================
static void _drawSplash() {
    tft.fillScreen(COL_PRIMARY);
    tft.setTextColor(COL_TEXT, COL_PRIMARY);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MONOPOLY", SCREEN_W / 2, SCREEN_H / 2 - 30, F_LG);
    tft.drawString("Electronic V2", SCREEN_W / 2, SCREEN_H / 2 + 10, F_MD);
    tft.setTextColor(COL_TEXT_DIM, COL_PRIMARY);
    tft.drawString("Loading...", SCREEN_W / 2, SCREEN_H / 2 + 40, F_SM);
    tft.setTextDatum(TL_DATUM);

    // Progress bar outline
    tft.drawRoundRect(60, SCREEN_H / 2 + 55, 200, 12, 4, COL_TEXT);
    _splashStart = millis();
}

static void _updateSplash() {
    uint32_t elapsed = millis() - _splashStart;
    int16_t progress = map(elapsed, 0, SPLASH_DURATION_MS, 0, 196);
    progress = constrain(progress, 0, 196);
    tft.fillRoundRect(62, SCREEN_H / 2 + 57, progress, 8, 2, COL_ACCENT);

    if (elapsed >= SPLASH_DURATION_MS || _btn == BTN_CENTER || _tp.pressed) {
        G.phase = PHASE_MENU;
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: MAIN MENU
// =============================================================================
static void _drawMenu() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("MONOPOLY Electronic");

    // Four big buttons in a 2×2 grid
    // Positions: top-left, top-right, bottom-left, bottom-right
}

static void _updateMenu() {
    const int16_t bx1 = 20, bx2 = 170;
    const int16_t by1 = 45, by2 = 140;
    const int16_t bw = 130, bh = 75;

    bool hasResume = storage_hasSavedGame();

    // Draw buttons
    if (ui_drawButton(bx1, by1, bw, bh, "NEW GAME", COL_BTN_BG, COL_TEXT, _menuSel == 0)) {
        hw_playSuccess();
        G.phase = PHASE_SETUP_COUNT;
        _needRedraw = true;
        return;
    }
    {
        uint16_t bg2 = hasResume ? COL_BTN_BG : RGB565(30, 30, 30);
        uint16_t fg2 = hasResume ? COL_TEXT : COL_TEXT_DIM;
        if (ui_drawButton(bx2, by1, bw, bh, "RESUME", bg2, fg2, _menuSel == 1)) {
            if (hasResume) {
                storage_loadGame();
                hw_playSuccess();
                _needRedraw = true;
            } else {
                hw_playError();
            }
            return;
        }
    }
    if (ui_drawButton(bx1, by2, bw, bh, "PROGRAM", COL_BTN_BG, COL_TEXT, _menuSel == 2)) {
        G.phase = PHASE_PROGRAMMING;
        _progMode = 0;
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(bx2, by2, bw, bh, "SETTINGS", COL_BTN_BG, COL_TEXT, _menuSel == 3)) {
        G.phase = PHASE_SETTINGS;
        _settSel = 0;
        _needRedraw = true;
        return;
    }

    // Button navigation
    if (_btn == BTN_LEFT)   _menuSel = (_menuSel + 3) % 4;
    if (_btn == BTN_RIGHT)  _menuSel = (_menuSel + 1) % 4;
    if (_btn == BTN_CENTER) {
        switch (_menuSel) {
            case 0: G.phase = PHASE_SETUP_COUNT; _needRedraw = true; break;
            case 1: if (hasResume) { storage_loadGame(); _needRedraw = true; } break;
            case 2: G.phase = PHASE_PROGRAMMING; _progMode = 0; _needRedraw = true; break;
            case 3: G.phase = PHASE_SETTINGS; _settSel = 0; _needRedraw = true; break;
        }
    }
}

// =============================================================================
//  SCREEN: SETUP – PLAYER COUNT
// =============================================================================
static void _drawSetupCount() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("New Game - Players");
}

static void _updateSetupCount() {
    // Big number display
    tft.fillRect(100, 60, 120, 60, COL_BG);
    char buf[4];
    snprintf(buf, 4, "%d", _setupCount);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, SCREEN_W / 2, 90, F_LG);
    tft.setTextDatum(TL_DATUM);

    // − and + buttons
    if (ui_drawButton(40, 70, 50, 40, "-", COL_BTN_BG, COL_TEXT)) {
        if (_setupCount > 2) { _setupCount--; hw_playDiceRoll(); }
    }
    if (ui_drawButton(230, 70, 50, 40, "+", COL_BTN_BG, COL_TEXT)) {
        if (_setupCount < MAX_PLAYERS) { _setupCount++; hw_playDiceRoll(); }
    }

    // Continue button
    if (ui_drawButton(80, 150, 160, 45, "CONTINUE", COL_BTN_ACTIVE, COL_TEXT)) {
        game_newGame(_setupCount);
        G.phase = PHASE_SETUP_PLAYERS;
        _setupRegistered = 0;
        _setupNfcScanning = true;
        _needRedraw = true;
        return;
    }
    // Back
    if (ui_drawButton(10, 205, 70, 28, "BACK", COL_DANGER, COL_TEXT)) {
        G.phase = PHASE_MENU;
        _needRedraw = true;
    }

    // Physical buttons
    if (_btn == BTN_LEFT  && _setupCount > 2) { _setupCount--; hw_playDiceRoll(); }
    if (_btn == BTN_RIGHT && _setupCount < MAX_PLAYERS) { _setupCount++; hw_playDiceRoll(); }
    if (_btn == BTN_CENTER) {
        game_newGame(_setupCount);
        G.phase = PHASE_SETUP_PLAYERS;
        _setupRegistered = 0;
        _setupNfcScanning = true;
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: SETUP – PLAYER REGISTRATION (NFC)
// =============================================================================
static uint32_t _nfcLastPoll = 0;

static void _drawSetupPlayers() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Scan Player Cards");
}

static void _updateSetupPlayers() {
    // Player slots
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        int16_t x = 10 + (i % 4) * 78;
        int16_t y = 38 + (i / 4) * 55;
        bool registered = (i < _setupRegistered);
        uint16_t bg = registered ? RGB565(20, 60, 20) : COL_BG_DARK;
        tft.fillRoundRect(x, y, 72, 48, 4, bg);
        tft.drawRoundRect(x, y, 72, 48, 4, G.players[i].colour);

        tft.setTextColor(COL_TEXT, bg);
        tft.setTextDatum(MC_DATUM);
        if (registered) {
            char buf[MAX_NAME_LEN + 1];
            snprintf(buf, sizeof(buf), "%s", G.players[i].name);
            tft.drawString(buf, x + 36, y + 16, F_SM);
            tft.setTextColor(COL_BTN_ACTIVE, bg);
            tft.drawString("OK", x + 36, y + 34, F_SM);
        } else if (i == _setupRegistered) {
            tft.drawString("SCAN", x + 36, y + 18, F_MD);
            tft.drawString("NOW", x + 36, y + 35, F_SM);
        } else {
            char num[4];
            snprintf(num, 4, "P%d", i + 1);
            tft.drawString(num, x + 36, y + 24, F_MD);
        }
        tft.setTextDatum(TL_DATUM);

        // Tap to rename registered player
        if (registered && ui_tapped(x, y, 72, 48)) {
            char newName[MAX_NAME_LEN + 1] = "";
            if (ui_keyboard(newName, MAX_NAME_LEN, "Rename Player")) {
                strncpy(G.players[i].name, newName, MAX_NAME_LEN);
            }
            _needRedraw = true;
            return;
        }
    }

    // Instruction
    tft.setTextColor(COL_TEXT_DIM, COL_BG);
    tft.setTextDatum(MC_DATUM);
    if (_setupRegistered < G.numPlayers) {
        tft.drawString("Scan card or tap SKIP", SCREEN_W / 2, 160, F_SM);
    }
    tft.setTextDatum(TL_DATUM);

    // NFC polling
    if (_setupRegistered < G.numPlayers && millis() - _nfcLastPoll > NFC_POLL_INTERVAL_MS) {
        _nfcLastPoll = millis();
        uint8_t uid[7];
        uint8_t uidLen;
        if (nfc_pollCard(uid, &uidLen, 100)) {
            Player& p = G.players[_setupRegistered];
            memcpy(p.uid, uid, uidLen);
            p.uidLen = uidLen;
            // Try to read player card data
            NfcPlayerCard card;
            if (nfc_readPlayerCard(uid, uidLen, card)) {
                strncpy(p.name, card.name, MAX_NAME_LEN);
                p.colour = card.colour;
            }
            hw_playSuccess();
            _setupRegistered++;
            _needRedraw = true;
        }
    }

    // Skip button (register without NFC)
    if (_setupRegistered < G.numPlayers) {
        if (ui_drawButton(110, 175, 100, 32, "SKIP", COL_BTN_BG, COL_TEXT)) {
            _setupRegistered++;
            hw_playCashIn();
            _needRedraw = true;
        }
    }

    // Start game when all registered
    if (_setupRegistered >= G.numPlayers) {
        if (ui_drawButton(80, 180, 160, 45, "START GAME", COL_BTN_ACTIVE, COL_TEXT)) {
            G.phase = PHASE_TURN_START;
            game_startTurn();
            _needRedraw = true;
        }
        if (_btn == BTN_CENTER) {
            G.phase = PHASE_TURN_START;
            game_startTurn();
            _needRedraw = true;
        }
    }

    // Back
    if (ui_drawButton(10, 210, 60, 24, "BACK", COL_DANGER, COL_TEXT)) {
        G.phase = PHASE_SETUP_COUNT;
        _needRedraw = true;
    }
    if (_btn == BTN_CENTER && _setupRegistered < G.numPlayers) {
        _setupRegistered++;
        hw_playCashIn();
    }
}

// =============================================================================
//  SCREEN: GAME – TURN START  (waiting for dice roll)
// =============================================================================
static void _drawTurnStart() {
    tft.fillScreen(COL_BG);
}

static void _updateTurnStart() {
    const Player& p = G.players[G.currentPlayer];

    // Header with player info
    tft.fillRect(0, 0, SCREEN_W, 30, p.colour);
    tft.setTextColor(COL_TEXT, p.colour);
    tft.setTextDatum(ML_DATUM);
    char buf[40];
    snprintf(buf, sizeof(buf), " %s's Turn", p.name);
    tft.drawString(buf, 4, 15, F_MD);
    snprintf(buf, sizeof(buf), "$%ld ", (long)p.money);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(buf, SCREEN_W - 4, 15, F_MD);
    tft.setTextDatum(TL_DATUM);

    // Position info
    tft.setTextColor(COL_TEXT_DIM, COL_BG);
    snprintf(buf, sizeof(buf), "Pos: %s (#%d)", TILES[p.position].name, p.position);
    tft.drawString(buf, 10, 35, F_SM);

    if (G.isDoubles && p.doublesCount > 0) {
        snprintf(buf, sizeof(buf), "Doubles! (%d)", p.doublesCount);
        tft.setTextColor(COL_WARN, COL_BG);
        tft.drawString(buf, 10, 47, F_SM);
    }

    // Big "ROLL DICE" area
    tft.fillRoundRect(60, 70, 200, 80, 10, COL_BTN_BG);
    tft.drawRoundRect(60, 70, 200, 80, 10, p.colour);
    tft.setTextColor(COL_TEXT, COL_BTN_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("TAP TO ROLL", SCREEN_W / 2, 100, F_MD);
    // Small dice preview
    ui_drawDie(130, 128, 10, random(1, 7), p.colour);
    ui_drawDie(190, 128, 10, random(1, 7), p.colour);
    tft.setTextDatum(TL_DATUM);

    // Bottom action bar
    int16_t ay = 165;
    if (ui_drawButton(5, ay, 60, 30, "Trade", COL_BTN_BG, COL_TEXT)) {
        G.phase = PHASE_TRADE_SELECT;
        _tradeSel = 0;
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(70, ay, 60, 30, "Props", COL_BTN_BG, COL_TEXT)) {
        _propListScroll = 0;
        _propListSel = -1;
        // Show properties of current player (reuse quick menu)
        G.phase = PHASE_QUICK_MENU;
        _qmSel = 0;
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(135, ay, 60, 30, "Save", COL_BTN_BG, COL_TEXT)) {
        storage_saveGame();
        hw_playSuccess();
        tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Saved!", SCREEN_W / 2, 210, F_SM);
        tft.setTextDatum(TL_DATUM);
        delay(600);
        _needRedraw = true;
    }
    if (ui_drawButton(200, ay, 60, 30, "Menu", COL_DANGER, COL_TEXT)) {
        G.phase = PHASE_QUICK_MENU;
        _qmSel = 0;
        _needRedraw = true;
        return;
    }

    // Other players bar at bottom
    int16_t py = 205;
    int16_t px = 2;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        if (i == G.currentPlayer) continue;
        if (!G.players[i].alive) continue;
        ui_drawPlayerMini(px, py, i);
        px += 76;
        if (px > SCREEN_W - 72) break; // clip
    }

    // Roll trigger
    bool rollTrigger = ui_tapped(60, 70, 200, 80) || (_btn == BTN_CENTER);
    if (rollTrigger) {
        G.phase = PHASE_ROLLING;
        _diceAnimStart = millis();
        hw_playDiceRoll();
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: DICE ROLLING ANIMATION
// =============================================================================
static void _drawRolling() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Rolling...");
}

static void _updateRolling() {
    uint32_t elapsed = millis() - _diceAnimStart;
    uint16_t animDuration = DICE_ANIM_MS;

    // Animate random faces
    if (elapsed < animDuration) {
        if (elapsed % 100 < 50) {
            _animD1 = random(1, 7);
            _animD2 = random(1, 7);
        }
        uint16_t col = G.players[G.currentPlayer].colour;
        ui_drawDie(SCREEN_W / 2 - 40, SCREEN_H / 2 - 10, 25, _animD1, col);
        ui_drawDie(SCREEN_W / 2 + 40, SCREEN_H / 2 - 10, 25, _animD2, col);
    } else {
        // Final result
        game_rollDice();
        uint16_t col = G.players[G.currentPlayer].colour;
        tft.fillRect(40, 60, 240, 120, COL_BG);
        ui_drawDie(SCREEN_W / 2 - 40, SCREEN_H / 2 - 20, 30, G.dice1, col);
        ui_drawDie(SCREEN_W / 2 + 40, SCREEN_H / 2 - 20, 30, G.dice2, col);

        char buf[20];
        snprintf(buf, sizeof(buf), "Total: %d", G.dice1 + G.dice2);
        tft.setTextColor(COL_ACCENT, COL_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(buf, SCREEN_W / 2, SCREEN_H / 2 + 40, F_MD);
        if (G.isDoubles) {
            tft.setTextColor(COL_WARN, COL_BG);
            tft.drawString("DOUBLES!", SCREEN_W / 2, SCREEN_H / 2 + 60, F_MD);
        }
        tft.setTextDatum(TL_DATUM);

        delay(800);
        game_movePlayer();
        if (G.phase == PHASE_MOVED) {
            game_resolveTile();
        }
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: TILE ACTION
// =============================================================================
static void _drawTileAction() {
    tft.fillScreen(COL_BG);
}

static void _updateTileAction() {
    const Player& p = G.players[G.currentPlayer];
    const TileData& tile = TILES[p.position];
    uint16_t gc = (tile.group != GROUP_NONE) ? groupColour(tile.group) : COL_PRIMARY;

    // Tile colour strip
    tft.fillRect(0, 0, SCREEN_W, 8, gc);

    // Tile name
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(tile.name, SCREEN_W / 2, 24, F_MD);
    tft.setTextDatum(TL_DATUM);

    char buf[48];

    switch (G.tileAction) {
        case ACT_BUY: {
            tft.setTextColor(COL_TEXT_DIM, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Unowned Property", SCREEN_W / 2, 44, F_SM);
            snprintf(buf, sizeof(buf), "Price: $%d", tile.price);
            tft.setTextColor(COL_ACCENT, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 64, F_MD);

            // Rent preview
            snprintf(buf, sizeof(buf), "Base rent: $%d", tile.rent[0]);
            tft.setTextColor(COL_TEXT_DIM, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 84, F_SM);
            tft.setTextDatum(TL_DATUM);

            bool canBuy = (p.money >= tile.price);
            uint16_t buyCol = canBuy ? COL_BTN_ACTIVE : RGB565(60, 60, 60);

            if (ui_drawButton(30, 110, 120, 45, "BUY", buyCol, COL_TEXT)) {
                if (canBuy) {
                    game_buyProperty(G.currentPlayer, p.position);
                    hw_playCashOut();
                    game_endTurn();
                    _needRedraw = true;
                } else {
                    hw_playError();
                }
                return;
            }
            if (ui_drawButton(170, 110, 120, 45, "SKIP", COL_BTN_BG, COL_TEXT)) {
                game_endTurn();
                _needRedraw = true;
                return;
            }
            if (_btn == BTN_CENTER && canBuy) {
                game_buyProperty(G.currentPlayer, p.position);
                hw_playCashOut();
                game_endTurn();
                _needRedraw = true;
            } else if (_btn == BTN_RIGHT) {
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_PAY_RENT: {
            int8_t owner = G.props[p.position].owner;
            int32_t rent = game_calcRent(p.position, G.dice1 + G.dice2);

            tft.setTextColor(COL_DANGER, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Rent Due!", SCREEN_W / 2, 48, F_MD);

            snprintf(buf, sizeof(buf), "Owner: %s", G.players[owner].name);
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 72, F_SM);

            snprintf(buf, sizeof(buf), "$%ld", (long)rent);
            tft.setTextColor(COL_ACCENT, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 96, F_LG);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(80, 130, 160, 45, "PAY RENT", COL_DANGER, COL_TEXT)) {
                game_payRent(G.currentPlayer, p.position);
                hw_playCashOut();
                game_endTurn();
                _needRedraw = true;
                return;
            }
            if (_btn == BTN_CENTER) {
                game_payRent(G.currentPlayer, p.position);
                hw_playCashOut();
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_OWN_PROP: {
            tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Your Property!", SCREEN_W / 2, 48, F_MD);

            uint8_t h = G.props[p.position].houses;
            if (h == 5) snprintf(buf, sizeof(buf), "HOTEL");
            else        snprintf(buf, sizeof(buf), "Houses: %d", h);
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 72, F_SM);

            if (tile.type == TILE_PROPERTY && h < MAX_HOUSES
                && game_ownsFullGroup(G.currentPlayer, tile.group)) {
                snprintf(buf, sizeof(buf), "Upgrade: $%d", tile.houseCost);
                tft.setTextColor(COL_ACCENT, COL_BG);
                tft.drawString(buf, SCREEN_W / 2, 92, F_SM);
                tft.setTextDatum(TL_DATUM);

                if (ui_drawButton(20, 115, 130, 40, "UPGRADE", COL_BTN_ACTIVE, COL_TEXT)) {
                    if (game_buildHouse(G.currentPlayer, p.position)) {
                        hw_playSuccess();
                        _needRedraw = true;
                    } else {
                        hw_playError();
                    }
                    return;
                }
            }
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(170, 115, 130, 40, "CONTINUE", COL_BTN_BG, COL_TEXT)) {
                game_endTurn();
                _needRedraw = true;
                return;
            }
            if (_btn == BTN_CENTER) {
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_TAX: {
            int32_t tax = tile.price; // price field holds tax amount
            tft.setTextColor(COL_DANGER, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString(tile.name, SCREEN_W / 2, 55, F_MD);
            snprintf(buf, sizeof(buf), "$%ld", (long)tax);
            tft.setTextColor(COL_ACCENT, COL_BG);
            tft.drawString(buf, SCREEN_W / 2, 85, F_LG);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(80, 120, 160, 45, "PAY TAX", COL_DANGER, COL_TEXT)) {
                game_payBank(G.currentPlayer, tax);
                hw_playCashOut();
                game_endTurn();
                _needRedraw = true;
                return;
            }
            if (_btn == BTN_CENTER) {
                game_payBank(G.currentPlayer, tax);
                hw_playCashOut();
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_FREE_PARKING: {
            tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Free Parking!", SCREEN_W / 2, 60, F_MD);
            if (G.settings.freeParkingPool && G.freeParkingPool > 0) {
                snprintf(buf, sizeof(buf), "Collect $%ld!", (long)G.freeParkingPool);
                tft.setTextColor(COL_ACCENT, COL_BG);
                tft.drawString(buf, SCREEN_W / 2, 90, F_MD);
            } else {
                tft.setTextColor(COL_TEXT_DIM, COL_BG);
                tft.drawString("Nothing happens", SCREEN_W / 2, 90, F_SM);
            }
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(80, 120, 160, 45, "CONTINUE", COL_BTN_ACTIVE, COL_TEXT) || _btn == BTN_CENTER) {
                if (G.settings.freeParkingPool && G.freeParkingPool > 0) {
                    G.players[G.currentPlayer].money += G.freeParkingPool;
                    G.freeParkingPool = 0;
                    hw_playCashIn();
                }
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_GO_TO_JAIL: {
            tft.setTextColor(COL_DANGER, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("GO TO JAIL!", SCREEN_W / 2, 80, F_LG);
            tft.setTextColor(COL_TEXT_DIM, COL_BG);
            tft.drawString("Do not pass GO", SCREEN_W / 2, 110, F_SM);
            tft.drawString("Do not collect $200", SCREEN_W / 2, 124, F_SM);
            tft.setTextDatum(TL_DATUM);

            hw_playJail();

            if (ui_drawButton(80, 150, 160, 40, "OK", COL_DANGER, COL_TEXT) || _btn == BTN_CENTER) {
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        case ACT_JUST_VISITING: {
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Just Visiting!", SCREEN_W / 2, 80, F_MD);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(80, 120, 160, 40, "CONTINUE", COL_BTN_BG, COL_TEXT) || _btn == BTN_CENTER) {
                game_endTurn();
                _needRedraw = true;
            }
            break;
        }

        default:
            // GO or other no-action tiles
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString(tile.name, SCREEN_W / 2, 80, F_MD);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(80, 120, 160, 40, "CONTINUE", COL_BTN_BG, COL_TEXT) || _btn == BTN_CENTER) {
                game_endTurn();
                _needRedraw = true;
            }
            break;
    }

    // Always show money at bottom
    snprintf(buf, sizeof(buf), "$%ld", (long)p.money);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextDatum(BR_DATUM);
    tft.drawString(buf, SCREEN_W - 5, SCREEN_H - 5, F_MD);
    tft.setTextDatum(TL_DATUM);
}

// =============================================================================
//  SCREEN: CARD DRAW  (Chance / Community Chest)
// =============================================================================
static void _drawCardDraw() {
    tft.fillScreen(COL_BG);
}

static void _updateCardDraw() {
    const CardData& card = G.cardIsChance
        ? CHANCE_CARDS[G.cardIndex]
        : COMMUNITY_CARDS[G.cardIndex];

    const char* title = G.cardIsChance ? "CHANCE" : "COMMUNITY CHEST";
    uint16_t titleCol = G.cardIsChance ? COL_WARN : RGB565(0, 150, 220);

    // Card frame
    tft.fillRoundRect(20, 10, 280, 160, 8, RGB565(255, 250, 230));
    tft.drawRoundRect(20, 10, 280, 160, 8, titleCol);
    tft.fillRect(20, 10, 280, 30, titleCol);

    tft.setTextColor(COL_TEXT, titleCol);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, SCREEN_W / 2, 25, F_MD);

    // Card text (handle line breaks)
    tft.setTextColor(TFT_BLACK, RGB565(255, 250, 230));
    int16_t ty = 55;
    char textBuf[128];
    strncpy(textBuf, card.text, sizeof(textBuf) - 1);
    textBuf[sizeof(textBuf) - 1] = 0;
    char* line = strtok(textBuf, "\n");
    while (line && ty < 150) {
        tft.drawString(line, SCREEN_W / 2, ty, F_MD);
        ty += 22;
        line = strtok(nullptr, "\n");
    }
    tft.setTextDatum(TL_DATUM);

    // Apply button
    if (ui_drawButton(80, 180, 160, 45, "OK", COL_BTN_ACTIVE, COL_TEXT) || _btn == BTN_CENTER) {
        hw_playCardDraw();
        game_applyCard(card);
        // Check if card moved player to a new tile that needs resolution
        if (card.effect == CARD_MOVETO || card.effect == CARD_MOVEREL
            || card.effect == CARD_NEAREST_RR || card.effect == CARD_NEAREST_UTIL) {
            // Resolve the new tile only if it's a property/railroad/utility
            const TileData& newTile = TILES[G.players[G.currentPlayer].position];
            if (card.effect != CARD_GO_JAIL &&
                (newTile.type == TILE_PROPERTY || newTile.type == TILE_RAILROAD
                 || newTile.type == TILE_UTILITY)) {
                game_resolveTile();
            } else {
                game_endTurn();
            }
        } else {
            game_endTurn();
        }
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: JAIL TURN
// =============================================================================
static void _drawJailTurn() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("IN JAIL");
}

static void _updateJailTurn() {
    const Player& p = G.players[G.currentPlayer];
    char buf[40];

    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextDatum(MC_DATUM);
    snprintf(buf, sizeof(buf), "%s is in Jail", p.name);
    tft.drawString(buf, SCREEN_W / 2, 44, F_MD);
    snprintf(buf, sizeof(buf), "Turn %d of %d", p.jailTurns + 1, G.settings.jailMaxTurns);
    tft.setTextColor(COL_TEXT_DIM, COL_BG);
    tft.drawString(buf, SCREEN_W / 2, 64, F_SM);
    tft.setTextDatum(TL_DATUM);

    // Option 1: Roll doubles
    if (ui_drawButton(10, 85, 145, 40, "ROLL DOUBLES", COL_BTN_BG, COL_TEXT)) {
        bool freed = game_tryJailRoll(G.currentPlayer);
        if (freed) {
            hw_playSuccess();
            // If freed by doubles, move normally
            game_movePlayer();
            if (G.phase == PHASE_MOVED) game_resolveTile();
        } else {
            hw_playError();
            tft.setTextColor(COL_DANGER, COL_BG);
            tft.setTextDatum(MC_DATUM);
            snprintf(buf, sizeof(buf), "Rolled %d & %d - No doubles", G.dice1, G.dice2);
            tft.drawString(buf, SCREEN_W / 2, 200, F_SM);
            tft.setTextDatum(TL_DATUM);
            delay(1200);
            game_endTurn();
        }
        _needRedraw = true;
        return;
    }

    // Option 2: Pay fine
    snprintf(buf, sizeof(buf), "PAY $%d", JAIL_FINE);
    if (ui_drawButton(165, 85, 145, 40, buf, COL_DANGER, COL_TEXT)) {
        game_payJailFine(G.currentPlayer);
        hw_playCashOut();
        // Now roll and move normally
        G.phase = PHASE_TURN_START;
        _needRedraw = true;
        return;
    }

    // Option 3: Use card
    if (p.hasJailCard) {
        if (ui_drawButton(10, 135, 300, 35, "USE GET-OUT-OF-JAIL CARD", COL_BTN_ACTIVE, COL_TEXT)) {
            game_useJailCard(G.currentPlayer);
            hw_playSuccess();
            G.phase = PHASE_TURN_START;
            _needRedraw = true;
            return;
        }
    }

    // Button controls
    if (_btn == BTN_CENTER) {
        // Default: roll
        bool freed = game_tryJailRoll(G.currentPlayer);
        if (freed) {
            hw_playSuccess();
            game_movePlayer();
            if (G.phase == PHASE_MOVED) game_resolveTile();
        } else {
            hw_playError();
            delay(800);
            game_endTurn();
        }
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: TRADE
// =============================================================================
static void _drawTradeSelect() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Trade - Select Player");
}

static void _updateTradeSelect() {
    // Show other players to trade with
    uint8_t shown = 0;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        if (i == G.currentPlayer || !G.players[i].alive) continue;
        int16_t y = 40 + shown * 38;
        bool sel = (_tradeSel == shown);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s  $%ld", G.players[i].name, (long)G.players[i].money);
        if (ui_drawButton(20, y, 280, 32, buf, COL_BTN_BG,
                         sel ? COL_ACCENT : COL_TEXT, sel)) {
            G.tradeWith = i;
            G.tradeMoneyOffer = 0;
            G.tradeMoneyRequest = 0;
            G.tradePropsOffer = 0;
            G.tradePropsRequest = 0;
            G.phase = PHASE_TRADE_OFFER;
            _needRedraw = true;
            return;
        }
        shown++;
    }

    // Back button
    if (ui_drawButton(10, 210, 70, 25, "BACK", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
        G.phase = PHASE_TURN_START;
        _needRedraw = true;
    }
    if (_btn == BTN_RIGHT) _tradeSel = (_tradeSel + 1) % max((int)shown, 1);
}

static void _drawTradeOffer() {
    tft.fillScreen(COL_BG);
}

static void _updateTradeOffer() {
    const Player& me   = G.players[G.currentPlayer];
    const Player& them = G.players[G.tradeWith];
    char buf[40];

    tft.setTextColor(me.colour, COL_BG);
    snprintf(buf, sizeof(buf), "You offer $%ld", (long)G.tradeMoneyOffer);
    tft.drawString(buf, 10, 5, F_SM);

    tft.setTextColor(them.colour, COL_BG);
    snprintf(buf, sizeof(buf), "%s offers $%ld", them.name, (long)G.tradeMoneyRequest);
    tft.drawString(buf, 10, 18, F_SM);

    // +/- money offer
    if (ui_drawButton(10, 35, 30, 22, "-", COL_BTN_BG, COL_TEXT)) {
        if (G.tradeMoneyOffer >= 50) G.tradeMoneyOffer -= 50;
        _needRedraw = true;
    }
    if (ui_drawButton(45, 35, 30, 22, "+", COL_BTN_BG, COL_TEXT)) {
        G.tradeMoneyOffer += 50;
        _needRedraw = true;
    }
    tft.setTextColor(COL_TEXT_DIM, COL_BG);
    tft.drawString("Your $", 80, 38, F_SM);

    if (ui_drawButton(130, 35, 30, 22, "-", COL_BTN_BG, COL_TEXT)) {
        if (G.tradeMoneyRequest >= 50) G.tradeMoneyRequest -= 50;
        _needRedraw = true;
    }
    if (ui_drawButton(165, 35, 30, 22, "+", COL_BTN_BG, COL_TEXT)) {
        G.tradeMoneyRequest += 50;
        _needRedraw = true;
    }
    tft.drawString("Their $", 200, 38, F_SM);

    // List my properties (selectable to offer)
    tft.setTextColor(me.colour, COL_BG);
    tft.drawString("Your props (tap=offer):", 10, 62, F_SM);
    int16_t py = 74;
    for (int i = 0; i < BOARD_SIZE && py < 140; i++) {
        if (G.props[i].owner != (int8_t)G.currentPlayer) continue;
        if (G.props[i].houses > 0) continue; // Can't trade with houses
        bool offered = (G.tradePropsOffer & (1ULL << i)) != 0;
        uint16_t col = offered ? COL_BTN_ACTIVE : COL_BTN_BG;
        if (ui_drawButton(10, py, 140, 18, TILES[i].name, col, COL_TEXT)) {
            G.tradePropsOffer ^= (1ULL << i);
            _needRedraw = true;
        }
        py += 20;
    }

    // List their properties (selectable to request)
    tft.setTextColor(them.colour, COL_BG);
    tft.drawString("Their props (tap=request):", 165, 62, F_SM);
    py = 74;
    for (int i = 0; i < BOARD_SIZE && py < 140; i++) {
        if (G.props[i].owner != (int8_t)G.tradeWith) continue;
        if (G.props[i].houses > 0) continue;
        bool requested = (G.tradePropsRequest & (1ULL << i)) != 0;
        uint16_t col = requested ? COL_WARN : COL_BTN_BG;
        if (ui_drawButton(165, py, 140, 18, TILES[i].name, col, COL_TEXT)) {
            G.tradePropsRequest ^= (1ULL << i);
            _needRedraw = true;
        }
        py += 20;
    }

    // Execute / Cancel
    if (ui_drawButton(20, 200, 120, 32, "EXECUTE", COL_BTN_ACTIVE, COL_TEXT)) {
        if (game_executeTrade()) {
            hw_playSuccess();
        } else {
            hw_playError();
        }
        G.phase = PHASE_TURN_START;
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(160, 200, 120, 32, "CANCEL", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
        G.phase = PHASE_TURN_START;
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: QUICK MENU
// =============================================================================
static void _drawQuickMenu() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Quick Menu");
}

static void _updateQuickMenu() {
    // All players overview
    int16_t y = 34;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        const Player& p = G.players[i];
        char buf[48];
        uint16_t bg = (i == G.currentPlayer) ? RGB565(30, 50, 30) : COL_BG_DARK;
        tft.fillRoundRect(5, y, 310, 22, 3, bg);
        tft.fillCircle(16, y + 11, 6, p.colour);
        snprintf(buf, sizeof(buf), "%-12s $%-6ld Pos:%d %s",
                 p.name, (long)p.money, p.position,
                 p.alive ? "" : "(OUT)");
        tft.setTextColor(p.alive ? COL_TEXT : COL_TEXT_DIM, bg);
        tft.drawString(buf, 26, y + 3, F_SM);
        y += 24;
    }

    y += 6;
    // View current player's properties
    const Player& me = G.players[G.currentPlayer];
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.drawString("Your properties:", 10, y, F_SM);
    y += 14;
    for (int i = 0; i < BOARD_SIZE && y < 210; i++) {
        if (G.props[i].owner != (int8_t)G.currentPlayer) continue;
        uint16_t gc = groupColour(TILES[i].group);
        tft.fillRect(10, y, 6, 10, gc);
        char buf[32];
        uint8_t h = G.props[i].houses;
        const char* hStr = (h == 5) ? " [H]" : "";
        snprintf(buf, sizeof(buf), "%s %s%s", TILES[i].name,
                 G.props[i].mortgaged ? "(M)" : "",
                 h > 0 && h < 5 ? "" : hStr);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.drawString(buf, 20, y, F_SM);
        if (h > 0 && h < 5) {
            char hBuf[8];
            snprintf(hBuf, sizeof(hBuf), " x%d", h);
            tft.setTextColor(COL_WARN, COL_BG);
            int16_t tw = tft.textWidth(buf, F_SM);
            tft.drawString(hBuf, 20 + tw, y, F_SM);
        }
        y += 12;
    }

    // Buttons at bottom
    if (ui_drawButton(5, 215, 70, 22, "BACK", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
        G.phase = PHASE_TURN_START;
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(80, 215, 80, 22, "END TURN", COL_BTN_BG, COL_TEXT)) {
        G.isDoubles = false; // Force end turn (no more re-rolls)
        game_endTurn();
        _needRedraw = true;
        return;
    }
    if (ui_drawButton(165, 215, 70, 22, "SAVE", COL_BTN_BG, COL_TEXT)) {
        storage_saveGame();
        hw_playSuccess();
    }
    if (ui_drawButton(240, 215, 70, 22, "QUIT", COL_DANGER, COL_TEXT)) {
        G.phase = PHASE_MENU;
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: PROGRAMMING MODE
// =============================================================================
static void _drawProgramming() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Programming Mode", COL_WARN);
}

static void _updateProgramming() {
    if (_progMode == 0) {
        // Select what to program
        if (ui_drawButton(40, 50, 240, 40, "Write Player Card", COL_BTN_BG, COL_TEXT, _progStep == 0)) {
            _progMode = 1; _progStep = 0; _needRedraw = true; return;
        }
        if (ui_drawButton(40, 100, 240, 40, "Write Property Card", COL_BTN_BG, COL_TEXT, _progStep == 1)) {
            _progMode = 2; _progStep = 0; _needRedraw = true; return;
        }
        if (ui_drawButton(40, 150, 240, 40, "Write Event Card", COL_BTN_BG, COL_TEXT, _progStep == 2)) {
            _progMode = 3; _progStep = 0; _needRedraw = true; return;
        }
        if (ui_drawButton(10, 210, 70, 25, "BACK", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
            G.phase = PHASE_MENU; _needRedraw = true;
        }
        return;
    }

    // ---------- Write Player Card Wizard ----------
    if (_progMode == 1) {
        static NfcPlayerCard _pCard;
        if (_progStep == 0) {
            // Enter player ID (0-7)
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Player ID (0-7):", SCREEN_W / 2, 50, F_MD);
            char buf[4];
            snprintf(buf, 4, "%d", _pCard.playerId);
            tft.drawString(buf, SCREEN_W / 2, 80, F_LG);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(60, 100, 40, 30, "-", COL_BTN_BG, COL_TEXT)) {
                if (_pCard.playerId > 0) _pCard.playerId--;
                _needRedraw = true;
            }
            if (ui_drawButton(220, 100, 40, 30, "+", COL_BTN_BG, COL_TEXT)) {
                if (_pCard.playerId < 7) _pCard.playerId++;
                _needRedraw = true;
            }

            _pCard.colour = PLAYER_COLOURS[_pCard.playerId];

            if (ui_drawButton(80, 150, 160, 35, "NEXT", COL_BTN_ACTIVE, COL_TEXT) || _btn == BTN_CENTER) {
                _progStep = 1; _needRedraw = true;
            }
        } else if (_progStep == 1) {
            // Enter name
            memset(_pCard.name, 0, sizeof(_pCard.name));
            ui_keyboard(_pCard.name, MAX_NAME_LEN, "Enter Player Name");
            _progStep = 2;
            _needRedraw = true;
        } else if (_progStep == 2) {
            // Scan card to write
            tft.setTextColor(COL_ACCENT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("SCAN CARD NOW", SCREEN_W / 2, 70, F_MD);
            char info[40];
            snprintf(info, sizeof(info), "ID:%d Name:%s", _pCard.playerId, _pCard.name);
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.drawString(info, SCREEN_W / 2, 100, F_SM);
            tft.setTextDatum(TL_DATUM);

            uint8_t uid[7]; uint8_t uidLen;
            if (nfc_pollCard(uid, &uidLen, 200)) {
                _pCard.type = NFC_TYPE_PLAYER;
                if (nfc_writePlayerCard(uid, uidLen, _pCard)) {
                    hw_playSuccess();
                    tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("SUCCESS!", SCREEN_W / 2, 140, F_MD);
                    tft.setTextDatum(TL_DATUM);
                    delay(1500);
                } else {
                    hw_playError();
                    tft.setTextColor(COL_DANGER, COL_BG);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("WRITE FAILED!", SCREEN_W / 2, 140, F_MD);
                    tft.setTextDatum(TL_DATUM);
                    delay(1500);
                }
                _progMode = 0; _needRedraw = true;
                return;
            }
            if (ui_drawButton(10, 210, 70, 25, "CANCEL", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
                _progMode = 0; _needRedraw = true;
            }
        }
        return;
    }

    // ---------- Write Property Card Wizard ----------
    if (_progMode == 2) {
        static uint8_t _propIdx = 0;
        if (_progStep == 0) {
            // Select property by index
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Select Property:", SCREEN_W / 2, 40, F_MD);

            // Show current property
            if (_propIdx < BOARD_SIZE && TILES[_propIdx].type != TILE_PROPERTY
                && TILES[_propIdx].type != TILE_RAILROAD
                && TILES[_propIdx].type != TILE_UTILITY) {
                // Skip non-property tiles
                _propIdx = (_propIdx + 1) % BOARD_SIZE;
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "#%d: %s", _propIdx, TILES[_propIdx].name);
            tft.drawString(buf, SCREEN_W / 2, 70, F_MD);
            uint16_t gc = groupColour(TILES[_propIdx].group);
            tft.fillRect(60, 90, 200, 6, gc);
            tft.setTextDatum(TL_DATUM);

            if (ui_drawButton(20, 110, 60, 35, "PREV", COL_BTN_BG, COL_TEXT)) {
                do { _propIdx = (_propIdx + BOARD_SIZE - 1) % BOARD_SIZE; }
                while (TILES[_propIdx].type != TILE_PROPERTY
                    && TILES[_propIdx].type != TILE_RAILROAD
                    && TILES[_propIdx].type != TILE_UTILITY);
                _needRedraw = true;
            }
            if (ui_drawButton(240, 110, 60, 35, "NEXT", COL_BTN_BG, COL_TEXT)) {
                do { _propIdx = (_propIdx + 1) % BOARD_SIZE; }
                while (TILES[_propIdx].type != TILE_PROPERTY
                    && TILES[_propIdx].type != TILE_RAILROAD
                    && TILES[_propIdx].type != TILE_UTILITY);
                _needRedraw = true;
            }
            if (ui_drawButton(100, 160, 120, 35, "WRITE", COL_BTN_ACTIVE, COL_TEXT) || _btn == BTN_CENTER) {
                _progStep = 1; _needRedraw = true;
            }
        } else if (_progStep == 1) {
            tft.setTextColor(COL_ACCENT, COL_BG);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("SCAN CARD NOW", SCREEN_W / 2, 70, F_MD);
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.drawString(TILES[_propIdx].name, SCREEN_W / 2, 100, F_MD);
            tft.setTextDatum(TL_DATUM);

            uint8_t uid[7]; uint8_t uidLen;
            if (nfc_pollCard(uid, &uidLen, 200)) {
                NfcPropertyCard card;
                card.type = NFC_TYPE_PROPERTY;
                card.tileIndex = _propIdx;
                card.group = TILES[_propIdx].group;
                strncpy(card.name, TILES[_propIdx].name, MAX_NAME_LEN);

                if (nfc_writePropertyCard(uid, uidLen, card)) {
                    hw_playSuccess();
                    tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("SUCCESS!", SCREEN_W / 2, 140, F_MD);
                    tft.setTextDatum(TL_DATUM);
                } else {
                    hw_playError();
                    tft.setTextColor(COL_DANGER, COL_BG);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("WRITE FAILED!", SCREEN_W / 2, 140, F_MD);
                    tft.setTextDatum(TL_DATUM);
                }
                delay(1500);
                _progMode = 0; _progStep = 0; _needRedraw = true;
                return;
            }
            if (ui_drawButton(10, 210, 70, 25, "CANCEL", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
                _progMode = 0; _progStep = 0; _needRedraw = true;
            }
        }
        return;
    }

    // ---------- Write Event Card ----------
    if (_progMode == 3) {
        tft.setTextColor(COL_ACCENT, COL_BG);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("SCAN EVENT CARD", SCREEN_W / 2, 70, F_MD);
        tft.setTextColor(COL_TEXT_DIM, COL_BG);
        tft.drawString("Card will be written as", SCREEN_W / 2, 100, F_SM);
        tft.drawString("generic event token", SCREEN_W / 2, 115, F_SM);
        tft.setTextDatum(TL_DATUM);

        uint8_t uid[7]; uint8_t uidLen;
        if (nfc_pollCard(uid, &uidLen, 200)) {
            NfcEventCard card;
            card.type = NFC_TYPE_EVENT;
            card.eventId = 0;
            if (nfc_writeEventCard(uid, uidLen, card)) {
                hw_playSuccess();
                tft.setTextColor(COL_BTN_ACTIVE, COL_BG);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("SUCCESS!", SCREEN_W / 2, 150, F_MD);
                tft.setTextDatum(TL_DATUM);
            } else {
                hw_playError();
            }
            delay(1500);
            _progMode = 0; _needRedraw = true;
            return;
        }
        if (ui_drawButton(10, 210, 70, 25, "CANCEL", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
            _progMode = 0; _needRedraw = true;
        }
    }
}

// =============================================================================
//  SCREEN: SETTINGS
// =============================================================================
static void _drawSettings() {
    tft.fillScreen(COL_BG);
    ui_drawHeader("Settings", RGB565(80, 80, 120));
}

static void _updateSettings() {
    GameSettings& s = G.settings;
    int16_t y = 36;
    char buf[40];

    auto settRow = [&](const char* label, const char* val, int idx) -> bool {
        bool sel = (_settSel == idx);
        uint16_t bg = sel ? RGB565(40, 40, 60) : COL_BG;
        tft.fillRect(0, y, SCREEN_W, 20, bg);
        tft.setTextColor(COL_TEXT, bg);
        tft.drawString(label, 10, y + 2, F_SM);
        tft.setTextColor(COL_ACCENT, bg);
        tft.setTextDatum(MR_DATUM);
        tft.drawString(val, SCREEN_W - 10, y + 10, F_SM);
        tft.setTextDatum(TL_DATUM);
        bool tap = ui_tapped(0, y, SCREEN_W, 20);
        y += 22;
        return tap;
    };

    snprintf(buf, sizeof(buf), "$%lu", (unsigned long)s.startingMoney);
    if (settRow("Starting Money", buf, 0)) {
        s.startingMoney = (s.startingMoney == 1500) ? 2000 :
                          (s.startingMoney == 2000) ? 2500 :
                          (s.startingMoney == 2500) ? 1000 : 1500;
        _needRedraw = true;
    }

    if (settRow("Free Parking Pool", s.freeParkingPool ? "ON" : "OFF", 1)) {
        s.freeParkingPool = !s.freeParkingPool;
        _needRedraw = true;
    }

    snprintf(buf, sizeof(buf), "%d turns", s.jailMaxTurns);
    if (settRow("Jail Max Turns", buf, 2)) {
        s.jailMaxTurns = (s.jailMaxTurns % 5) + 1;
        _needRedraw = true;
    }

    if (settRow("Auto Rent", s.autoRent ? "ON" : "OFF", 3)) {
        s.autoRent = !s.autoRent;
        _needRedraw = true;
    }

    if (settRow("Require NFC", s.nfcRequired ? "ON" : "OFF", 4)) {
        s.nfcRequired = !s.nfcRequired;
        _needRedraw = true;
    }

    snprintf(buf, sizeof(buf), "%d", s.diceSpeed);
    if (settRow("Dice Speed (1-3)", buf, 5)) {
        s.diceSpeed = (s.diceSpeed % 3) + 1;
        _needRedraw = true;
    }

    snprintf(buf, sizeof(buf), "%d", s.volume);
    if (settRow("Volume (0-5)", buf, 6)) {
        s.volume = (s.volume + 1) % 6;
        hw_setVolume(s.volume);
        _needRedraw = true;
    }

    // Save & Back
    if (ui_drawButton(20, 210, 120, 25, "SAVE", COL_BTN_ACTIVE, COL_TEXT)) {
        storage_saveSettings(s);
        hw_setVolume(s.volume);
        hw_playSuccess();
    }
    if (ui_drawButton(160, 210, 120, 25, "BACK", COL_DANGER, COL_TEXT) || _btn == BTN_LEFT) {
        G.phase = PHASE_MENU;
        _needRedraw = true;
    }

    // Button navigation
    if (_btn == BTN_RIGHT) { _settSel = (_settSel + 1) % 7; _needRedraw = true; }
    if (_btn == BTN_CENTER) {
        // Toggle current setting
        switch (_settSel) {
            case 0: s.startingMoney = (s.startingMoney == 1500) ? 2000 : (s.startingMoney == 2000) ? 2500 : (s.startingMoney == 2500) ? 1000 : 1500; break;
            case 1: s.freeParkingPool = !s.freeParkingPool; break;
            case 2: s.jailMaxTurns = (s.jailMaxTurns % 5) + 1; break;
            case 3: s.autoRent = !s.autoRent; break;
            case 4: s.nfcRequired = !s.nfcRequired; break;
            case 5: s.diceSpeed = (s.diceSpeed % 3) + 1; break;
            case 6: s.volume = (s.volume + 1) % 6; hw_setVolume(s.volume); break;
        }
        _needRedraw = true;
    }
}

// =============================================================================
//  SCREEN: GAME OVER
// =============================================================================
static void _drawGameOver() {
    tft.fillScreen(COL_BG);
}

static void _updateGameOver() {
    uint8_t w = game_getWinner();
    const Player& p = G.players[w];

    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("GAME OVER!", SCREEN_W / 2, 40, F_LG);

    tft.fillCircle(SCREEN_W / 2, 100, 20, p.colour);
    tft.setTextColor(COL_TEXT, COL_BG);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s WINS!", p.name);
    tft.drawString(buf, SCREEN_W / 2, 140, F_MD);
    snprintf(buf, sizeof(buf), "Final: $%ld", (long)p.money);
    tft.drawString(buf, SCREEN_W / 2, 165, F_SM);
    tft.setTextDatum(TL_DATUM);

    if (ui_drawButton(80, 195, 160, 40, "MAIN MENU", COL_BTN_ACTIVE, COL_TEXT) || _btn == BTN_CENTER) {
        storage_clearSave();
        G.phase = PHASE_MENU;
        _needRedraw = true;
    }
}

// =============================================================================
//  MASTER UI INIT / UPDATE
// =============================================================================
void ui_init() {
    _needRedraw = true;
    _splashStart = millis();
}

void ui_update() {
    static GamePhase _lastPhase = (GamePhase)-1;
    _frameTime = millis();
    _tp  = hw_getTouch();
    _btn = hw_readButtons();
    _touchConsumed = false;

    if (G.phase != _lastPhase) {
        DBG("UI phase: %d -> %d", (int)_lastPhase, (int)G.phase);
        _lastPhase = G.phase;
    }

#if DEBUG
    if (_tp.pressed) DBG("touch: x=%d y=%d", _tp.x, _tp.y);
    if (_btn != BTN_NONE) DBG("button: %d", (int)_btn);
#endif

    // Check for long-press center → programming mode (from any game screen)
    if (hw_isBtnHeld(BTN_CENTER, 3000) && G.phase >= PHASE_TURN_START && G.phase <= PHASE_QUICK_MENU) {
        // Ignored during game to avoid accidental triggers
    }

    // Draw phase (only on dirty)
    if (_needRedraw) {
        switch (G.phase) {
            case PHASE_SPLASH:         _drawSplash();        break;
            case PHASE_MENU:           _drawMenu();          break;
            case PHASE_SETUP_COUNT:    _drawSetupCount();    break;
            case PHASE_SETUP_PLAYERS:  _drawSetupPlayers();  break;
            case PHASE_TURN_START:     _drawTurnStart();     break;
            case PHASE_ROLLING:        _drawRolling();       break;
            case PHASE_MOVED:          /* no separate draw */ break;
            case PHASE_TILE_ACTION:    _drawTileAction();    break;
            case PHASE_CARD_DRAW:      _drawCardDraw();      break;
            case PHASE_JAIL_TURN:      _drawJailTurn();      break;
            case PHASE_TRADE_SELECT:   _drawTradeSelect();   break;
            case PHASE_TRADE_OFFER:    _drawTradeOffer();    break;
            case PHASE_QUICK_MENU:     _drawQuickMenu();     break;
            case PHASE_PROGRAMMING:    _drawProgramming();   break;
            case PHASE_SETTINGS:       _drawSettings();      break;
            case PHASE_GAME_OVER:      _drawGameOver();      break;
        }
        _needRedraw = false;
        G.screenDirty = false;
    }

    // Update phase (every frame)
    switch (G.phase) {
        case PHASE_SPLASH:         _updateSplash();        break;
        case PHASE_MENU:           _updateMenu();          break;
        case PHASE_SETUP_COUNT:    _updateSetupCount();    break;
        case PHASE_SETUP_PLAYERS:  _updateSetupPlayers();  break;
        case PHASE_TURN_START:     _updateTurnStart();     break;
        case PHASE_ROLLING:        _updateRolling();       break;
        case PHASE_MOVED:
            // Auto-resolve immediately
            game_resolveTile();
            _needRedraw = true;
            break;
        case PHASE_TILE_ACTION:    _updateTileAction();    break;
        case PHASE_CARD_DRAW:      _updateCardDraw();      break;
        case PHASE_JAIL_TURN:      _updateJailTurn();      break;
        case PHASE_TRADE_SELECT:   _updateTradeSelect();   break;
        case PHASE_TRADE_OFFER:    _updateTradeOffer();    break;
        case PHASE_QUICK_MENU:     _updateQuickMenu();     break;
        case PHASE_PROGRAMMING:    _updateProgramming();   break;
        case PHASE_SETTINGS:       _updateSettings();      break;
        case PHASE_GAME_OVER:      _updateGameOver();      break;
    }

    // If game logic set dirty, schedule redraw next frame
    if (G.screenDirty) _needRedraw = true;
}
