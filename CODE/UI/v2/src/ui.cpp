// =============================================================================
// MONOPOLY ELECTRONIC V2 — UI (LVGL implementation)
// =============================================================================
#include "ui.h"
#include "hardware.h"
#include "nfc_handler.h"
#include "storage.h"
#include "config.h"
#include <lvgl.h>

// =============================================================================
// FONTS
// =============================================================================
#define FONT_SM  &lv_font_montserrat_12
#define FONT_MD  &lv_font_montserrat_16
#define FONT_LG  &lv_font_montserrat_20
#define FONT_XL  &lv_font_montserrat_28

// =============================================================================
// COLOR HELPERS
// =============================================================================
// Convert RGB565 (game_data / config) to lv_color_t
static lv_color_t _c(uint16_t rgb565) {
    uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
    uint8_t g = ((rgb565 >> 5)  & 0x3F) << 2;
    uint8_t b = ( rgb565        & 0x1F) << 3;
    return lv_color_make(r, g, b);
}

// Theme colours as lv_color_t
static lv_color_t C_BG, C_BG_DARK, C_PRIMARY, C_ACCENT, C_TEXT, C_TEXT_DIM;
static lv_color_t C_BTN_BG, C_BTN_ACTIVE, C_DANGER, C_WARN, C_HEADER;

static void _initColors() {
    C_BG         = lv_color_hex(0x000000);
    C_BG_DARK    = lv_color_hex(0x101018);
    C_PRIMARY    = lv_color_hex(0x00643C);
    C_ACCENT     = lv_color_hex(0xFFC800);
    C_TEXT       = lv_color_hex(0xFFFFFF);
    C_TEXT_DIM   = lv_color_hex(0x7B7B7B);
    C_BTN_BG    = lv_color_hex(0x282832);
    C_BTN_ACTIVE = lv_color_hex(0x00B450);
    C_DANGER     = lv_color_hex(0xDC2828);
    C_WARN       = lv_color_hex(0xF0B400);
    C_HEADER     = lv_color_hex(0x141423);
}

static lv_color_t _playerColor(uint8_t idx) {
    return _c(PLAYER_COLOURS[idx % MAX_PLAYERS]);
}

static lv_color_t _groupColor(ColorGroup g) {
    return _c(groupColour(g));
}

// =============================================================================
// STATE
// =============================================================================
static GamePhase _prevPhase    = (GamePhase)0xFF;
static uint8_t   _setupCount   = 2;
static uint8_t   _setupRegistered = 0;
static uint8_t   _progMode     = 0;  // 0=select, 1=player, 2=property, 3=event
static uint8_t   _progStep     = 0;
static int8_t    _settSel      = 0;
static uint8_t   _propIdx      = 1;  // For programming mode property selection

// Timers
static lv_timer_t* _activeTimer  = nullptr;
static lv_timer_t* _activeTimer2 = nullptr;

static void _clearTimers() {
    if (_activeTimer)  { lv_timer_delete(_activeTimer);  _activeTimer  = nullptr; }
    if (_activeTimer2) { lv_timer_delete(_activeTimer2); _activeTimer2 = nullptr; }
}

// =============================================================================
// SCREEN MANAGEMENT
// =============================================================================
static void _showScreen(lv_obj_t* scr) {
    _clearTimers();
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

static lv_obj_t* _newScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    // Remove padding
    lv_obj_set_style_pad_all(scr, 0, 0);
    return scr;
}

// =============================================================================
// WIDGET HELPERS
// =============================================================================
static lv_obj_t* _mkHeader(lv_obj_t* parent, const char* text, lv_color_t bg) {
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, SCREEN_W, 28);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, bg, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);

    lv_obj_t* lbl = lv_label_create(bar);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, C_TEXT, 0);
    lv_obj_set_style_text_font(lbl, FONT_MD, 0);
    lv_obj_center(lbl);
    return bar;
}

static lv_obj_t* _mkBtn(lv_obj_t* parent, const char* text,
                          int16_t x, int16_t y, int16_t w, int16_t h,
                          lv_color_t bg, lv_event_cb_t cb, void* ud = nullptr) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    // Focus ring
    lv_obj_set_style_outline_width(btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(btn, C_ACCENT, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);

    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, C_TEXT, 0);
    lv_obj_set_style_text_font(lbl, FONT_MD, 0);
    lv_obj_center(lbl);

    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);
    lv_group_add_obj(lv_group_get_default(), btn);
    return btn;
}

// Label helper with position + font + color
static lv_obj_t* _mkLabel(lv_obj_t* parent, const char* text,
                            lv_align_t align, int16_t xofs, int16_t yofs,
                            const lv_font_t* font, lv_color_t color) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_align(lbl, align, xofs, yofs);
    return lbl;
}

// Player mini bar (small info strip)
static void _mkPlayerMini(lv_obj_t* parent, int16_t x, int16_t y, uint8_t idx) {
    if (idx >= G.numPlayers) return;
    const Player& p = G.players[idx];

    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 74, 24);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_color(cont, p.alive ? C_BG_DARK : lv_color_hex(0x3C1414), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cont, 3, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    // Color dot
    lv_obj_t* dot = lv_obj_create(cont);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_pos(dot, 2, 7);
    lv_obj_set_style_bg_color(dot, _c(p.colour), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);

    // Name
    lv_obj_t* nm = lv_label_create(cont);
    char nb[8]; snprintf(nb, sizeof(nb), "%.5s", p.name);
    lv_label_set_text(nm, nb);
    lv_obj_set_style_text_color(nm, C_TEXT, 0);
    lv_obj_set_style_text_font(nm, FONT_SM, 0);
    lv_obj_set_pos(nm, 14, 1);

    // Money
    lv_obj_t* mn = lv_label_create(cont);
    char mb[12]; snprintf(mb, sizeof(mb), "$%ld", (long)p.money);
    lv_label_set_text(mn, mb);
    lv_obj_set_style_text_color(mn, C_ACCENT, 0);
    lv_obj_set_style_text_font(mn, FONT_SM, 0);
    lv_obj_set_pos(mn, 14, 13);
}

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static void _buildSplash();
static void _buildMenu();
static void _buildSetupCount();
static void _buildSetupPlayers();
static void _buildTurnStart();
static void _buildRolling();
static void _buildTileAction();
static void _buildCardDraw();
static void _buildJailTurn();
static void _buildTradeSelect();
static void _buildTradeOffer();
static void _buildQuickMenu();
static void _buildProgramming();
static void _buildSettings();
static void _buildGameOver();

// =============================================================================
// SCREEN: SPLASH
// =============================================================================
static void _buildSplash() {
    lv_obj_t* scr = _newScreen();
    lv_obj_set_style_bg_color(scr, C_PRIMARY, 0);

    _mkLabel(scr, "MONOPOLY", LV_ALIGN_CENTER, 0, -30, FONT_XL, C_TEXT);
    _mkLabel(scr, "Electronic V2", LV_ALIGN_CENTER, 0, 10, FONT_MD, C_TEXT);
    _mkLabel(scr, "Loading...", LV_ALIGN_CENTER, 0, 40, FONT_SM, C_TEXT_DIM);

    // Progress bar
    lv_obj_t* bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 200, 12);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 65);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, C_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, C_ACCENT, LV_PART_INDICATOR);

    _showScreen(scr);

    // Animate progress bar, then go to menu
    static lv_obj_t* _barRef = bar;
    _barRef = bar;
    static int _splashProg = 0;
    _splashProg = 0;
    _activeTimer = lv_timer_create([](lv_timer_t* t) {
        _splashProg += 4;
        if (_splashProg > 100) _splashProg = 100;
        lv_bar_set_value(_barRef, _splashProg, LV_ANIM_ON);
        if (_splashProg >= 100) {
            G.phase = PHASE_MENU;
            G.screenDirty = true;
        }
    }, SPLASH_DURATION_MS / 25, nullptr);
}

// =============================================================================
// SCREEN: MAIN MENU
// =============================================================================
static void _evMenuNew(lv_event_t* e)     { hw_playSuccess(); G.phase = PHASE_SETUP_COUNT; G.screenDirty = true; }
static void _evMenuResume(lv_event_t* e)  {
    if (storage_hasSavedGame()) { storage_loadGame(); hw_playSuccess(); G.screenDirty = true; }
    else hw_playError();
}
static void _evMenuProgram(lv_event_t* e)  { _progMode = 0; G.phase = PHASE_PROGRAMMING; G.screenDirty = true; }
static void _evMenuSettings(lv_event_t* e) { _settSel = 0; G.phase = PHASE_SETTINGS; G.screenDirty = true; }

static void _buildMenu() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "MONOPOLY Electronic", C_PRIMARY);

    _mkBtn(scr, "NEW GAME", 20, 45, 130, 75, C_BTN_BG, _evMenuNew);

    bool hasResume = storage_hasSavedGame();
    lv_obj_t* rb = _mkBtn(scr, "RESUME", 170, 45, 130, 75,
                           hasResume ? C_BTN_BG : lv_color_hex(0x1E1E1E), _evMenuResume);
    if (!hasResume) lv_obj_set_style_text_opa(rb, LV_OPA_50, LV_PART_ITEMS);

    _mkBtn(scr, "PROGRAM", 20, 140, 130, 75, C_BTN_BG, _evMenuProgram);
    _mkBtn(scr, "SETTINGS", 170, 140, 130, 75, C_BTN_BG, _evMenuSettings);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: SETUP – PLAYER COUNT
// =============================================================================
static lv_obj_t* _countLabel = nullptr;

static void _evCountDec(lv_event_t* e) {
    if (_setupCount > 2) { _setupCount--; hw_playDiceRoll(); }
    char b[4]; snprintf(b, 4, "%d", _setupCount);
    lv_label_set_text(_countLabel, b);
}
static void _evCountInc(lv_event_t* e) {
    if (_setupCount < MAX_PLAYERS) { _setupCount++; hw_playDiceRoll(); }
    char b[4]; snprintf(b, 4, "%d", _setupCount);
    lv_label_set_text(_countLabel, b);
}
static void _evCountContinue(lv_event_t* e) {
    game_newGame(_setupCount);
    _setupRegistered = 0;
    G.phase = PHASE_SETUP_PLAYERS;
    G.screenDirty = true;
}
static void _evBack(lv_event_t* e) {
    GamePhase target = *(GamePhase*)lv_event_get_user_data(e);
    G.phase = target;
    G.screenDirty = true;
}

static GamePhase _phMenu = PHASE_MENU;

static void _buildSetupCount() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "New Game - Players", C_PRIMARY);

    _mkBtn(scr, LV_SYMBOL_MINUS, 40, 70, 50, 40, C_BTN_BG, _evCountDec);
    _mkBtn(scr, LV_SYMBOL_PLUS, 230, 70, 50, 40, C_BTN_BG, _evCountInc);

    char b[4]; snprintf(b, 4, "%d", _setupCount);
    _countLabel = _mkLabel(scr, b, LV_ALIGN_TOP_MID, 0, 72, FONT_XL, C_ACCENT);

    _mkBtn(scr, "CONTINUE", 80, 140, 160, 45, C_BTN_ACTIVE, _evCountContinue);
    _mkBtn(scr, "BACK", 10, 205, 70, 28, C_DANGER, _evBack, &_phMenu);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: SETUP – PLAYERS (NFC)
// =============================================================================
static lv_obj_t* _setupScr = nullptr;

static void _rebuildSetupPlayers();

static void _evSkipPlayer(lv_event_t* e) {
    _setupRegistered++;
    hw_playCashIn();
    _rebuildSetupPlayers();
}
static void _evStartGame(lv_event_t* e) {
    G.phase = PHASE_TURN_START;
    game_startTurn();
    G.screenDirty = true;
}
static GamePhase _phSetupCount = PHASE_SETUP_COUNT;

static void _rebuildSetupPlayers() {
    G.phase = PHASE_SETUP_PLAYERS;
    G.screenDirty = true;
}

static void _buildSetupPlayers() {
    lv_obj_t* scr = _newScreen();
    _setupScr = scr;
    _mkHeader(scr, "Scan Player Cards", C_PRIMARY);

    // Player slots
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        int16_t x = 10 + (i % 4) * 78;
        int16_t y = 38 + (i / 4) * 55;
        bool reg = (i < _setupRegistered);

        lv_obj_t* slot = lv_obj_create(scr);
        lv_obj_remove_style_all(slot);
        lv_obj_set_size(slot, 72, 48);
        lv_obj_set_pos(slot, x, y);
        lv_obj_set_style_bg_color(slot, reg ? lv_color_hex(0x143C14) : C_BG_DARK, 0);
        lv_obj_set_style_bg_opa(slot, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(slot, 4, 0);
        lv_obj_set_style_border_width(slot, 1, 0);
        lv_obj_set_style_border_color(slot, _c(G.players[i].colour), 0);
        lv_obj_set_scrollbar_mode(slot, LV_SCROLLBAR_MODE_OFF);

        if (reg) {
            lv_obj_t* nm = lv_label_create(slot);
            char nb[MAX_NAME_LEN+1]; snprintf(nb, sizeof(nb), "%s", G.players[i].name);
            lv_label_set_text(nm, nb);
            lv_obj_set_style_text_color(nm, C_TEXT, 0);
            lv_obj_set_style_text_font(nm, FONT_SM, 0);
            lv_obj_align(nm, LV_ALIGN_TOP_MID, 0, 4);

            lv_obj_t* ok = lv_label_create(slot);
            lv_label_set_text(ok, "OK");
            lv_obj_set_style_text_color(ok, C_BTN_ACTIVE, 0);
            lv_obj_set_style_text_font(ok, FONT_SM, 0);
            lv_obj_align(ok, LV_ALIGN_BOTTOM_MID, 0, -4);
        } else if (i == _setupRegistered) {
            _mkLabel(slot, "SCAN", LV_ALIGN_TOP_MID, 0, 6, FONT_MD, C_TEXT);
            _mkLabel(slot, "NOW", LV_ALIGN_BOTTOM_MID, 0, -6, FONT_SM, C_TEXT_DIM);
        } else {
            char num[4]; snprintf(num, 4, "P%d", i + 1);
            _mkLabel(slot, num, LV_ALIGN_CENTER, 0, 0, FONT_MD, C_TEXT);
        }
    }

    // Instructions
    if (_setupRegistered < G.numPlayers) {
        _mkLabel(scr, "Scan card or tap SKIP", LV_ALIGN_TOP_MID, 0, 155, FONT_SM, C_TEXT_DIM);
        _mkBtn(scr, "SKIP", 110, 172, 100, 32, C_BTN_BG, _evSkipPlayer);
    } else {
        _mkBtn(scr, "START GAME", 80, 172, 160, 45, C_BTN_ACTIVE, _evStartGame);
    }

    _mkBtn(scr, "BACK", 10, 210, 60, 24, C_DANGER, _evBack, &_phSetupCount);

    _showScreen(scr);

    // NFC polling timer
    if (_setupRegistered < G.numPlayers) {
        _activeTimer = lv_timer_create([](lv_timer_t* t) {
            if (_setupRegistered >= G.numPlayers) { return; }
            uint8_t uid[7]; uint8_t uidLen;
            if (nfc_pollCard(uid, &uidLen, 80)) {
                Player& p = G.players[_setupRegistered];
                memcpy(p.uid, uid, uidLen);
                p.uidLen = uidLen;
                NfcPlayerCard card;
                if (nfc_readPlayerCard(uid, uidLen, card)) {
                    strncpy(p.name, card.name, MAX_NAME_LEN);
                    p.colour = card.colour;
                }
                hw_playSuccess();
                _setupRegistered++;
                G.phase = PHASE_SETUP_PLAYERS;
                G.screenDirty = true;
            }
        }, NFC_POLL_INTERVAL_MS, nullptr);
    }
}

// =============================================================================
// SCREEN: TURN START
// =============================================================================
static void _evRollDice(lv_event_t* e) {
    G.phase = PHASE_ROLLING;
    hw_playDiceRoll();
    G.screenDirty = true;
}
static void _evTrade(lv_event_t* e)     { G.phase = PHASE_TRADE_SELECT; G.screenDirty = true; }
static void _evQuickMenu(lv_event_t* e) { G.phase = PHASE_QUICK_MENU;   G.screenDirty = true; }
static void _evSaveGame(lv_event_t* e)  { storage_saveGame(); hw_playSuccess(); }

static void _buildTurnStart() {
    lv_obj_t* scr = _newScreen();
    const Player& p = G.players[G.currentPlayer];

    // Player colour header
    lv_obj_t* hdr = lv_obj_create(scr);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, SCREEN_W, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, _c(p.colour), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);

    char buf[48];
    snprintf(buf, sizeof(buf), " %s's Turn", p.name);
    lv_obj_t* hn = lv_label_create(hdr);
    lv_label_set_text(hn, buf);
    lv_obj_set_style_text_color(hn, C_TEXT, 0);
    lv_obj_set_style_text_font(hn, FONT_MD, 0);
    lv_obj_align(hn, LV_ALIGN_LEFT_MID, 4, 0);

    snprintf(buf, sizeof(buf), "$%ld ", (long)p.money);
    lv_obj_t* hm = lv_label_create(hdr);
    lv_label_set_text(hm, buf);
    lv_obj_set_style_text_color(hm, C_TEXT, 0);
    lv_obj_set_style_text_font(hm, FONT_MD, 0);
    lv_obj_align(hm, LV_ALIGN_RIGHT_MID, -4, 0);

    // Position info
    snprintf(buf, sizeof(buf), "Pos: %s (#%d)", TILES[p.position].name, p.position);
    _mkLabel(scr, buf, LV_ALIGN_TOP_LEFT, 10, 34, FONT_SM, C_TEXT_DIM);

    if (G.isDoubles && p.doublesCount > 0) {
        snprintf(buf, sizeof(buf), "Doubles! (%d)", p.doublesCount);
        _mkLabel(scr, buf, LV_ALIGN_TOP_LEFT, 10, 48, FONT_SM, C_WARN);
    }

    // Big roll button
    _mkBtn(scr, "TAP TO ROLL", 60, 68, 200, 65, C_BTN_BG, _evRollDice);

    // Action bar
    _mkBtn(scr, "Trade", 5, 148, 60, 28, C_BTN_BG, _evTrade);
    _mkBtn(scr, "Props", 70, 148, 60, 28, C_BTN_BG, _evQuickMenu);
    _mkBtn(scr, "Save", 135, 148, 60, 28, C_BTN_BG, _evSaveGame);
    _mkBtn(scr, "Menu", 200, 148, 60, 28, C_DANGER, _evQuickMenu);

    // Other players
    int16_t px = 2, py = 186;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        if (i == G.currentPlayer || !G.players[i].alive) continue;
        _mkPlayerMini(scr, px, py, i);
        px += 76;
        if (px > SCREEN_W - 72) break;
    }

    _showScreen(scr);
}

// =============================================================================
// SCREEN: DICE ROLLING ANIMATION
// =============================================================================
static lv_obj_t* _diceL1 = nullptr;
static lv_obj_t* _diceL2 = nullptr;
static int _diceAnimCount = 0;

static void _buildRolling() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "Rolling...", C_PRIMARY);

    // Two dice "boxes"
    for (int i = 0; i < 2; i++) {
        lv_obj_t* box = lv_obj_create(scr);
        lv_obj_remove_style_all(box);
        lv_obj_set_size(box, 60, 60);
        lv_obj_set_pos(box, i == 0 ? 80 : 180, 70);
        lv_obj_set_style_bg_color(box, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(box, 8, 0);
        lv_obj_set_style_border_width(box, 2, 0);
        lv_obj_set_style_border_color(box, _c(G.players[G.currentPlayer].colour), 0);

        lv_obj_t* lbl = lv_label_create(box);
        lv_label_set_text(lbl, "?");
        lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
        lv_obj_set_style_text_font(lbl, FONT_XL, 0);
        lv_obj_center(lbl);

        if (i == 0) _diceL1 = lbl; else _diceL2 = lbl;
    }

    lv_obj_t* totalLbl = _mkLabel(scr, "", LV_ALIGN_CENTER, 0, 50, FONT_MD, C_ACCENT);
    lv_obj_t* doublesLbl = _mkLabel(scr, "", LV_ALIGN_CENTER, 0, 72, FONT_MD, C_WARN);

    _showScreen(scr);

    // Animation timer
    _diceAnimCount = 0;
    static lv_obj_t* _totalRef = totalLbl;
    static lv_obj_t* _doublesRef = doublesLbl;
    _totalRef = totalLbl;
    _doublesRef = doublesLbl;

    _activeTimer = lv_timer_create([](lv_timer_t* t) {
        _diceAnimCount++;
        int animTicks = 12;
        if (_diceAnimCount < animTicks) {
            char d1[4], d2[4];
            snprintf(d1, 4, "%d", (int)random(1, 7));
            snprintf(d2, 4, "%d", (int)random(1, 7));
            lv_label_set_text(_diceL1, d1);
            lv_label_set_text(_diceL2, d2);
        } else if (_diceAnimCount == animTicks) {
            game_rollDice();
            char d1[4], d2[4];
            snprintf(d1, 4, "%d", G.dice1);
            snprintf(d2, 4, "%d", G.dice2);
            lv_label_set_text(_diceL1, d1);
            lv_label_set_text(_diceL2, d2);

            char buf[20];
            snprintf(buf, sizeof(buf), "Total: %d", G.dice1 + G.dice2);
            lv_label_set_text(_totalRef, buf);
            if (G.isDoubles) lv_label_set_text(_doublesRef, "DOUBLES!");
        } else if (_diceAnimCount >= animTicks + 8) {
            // Advance game
            game_movePlayer();
            if (G.phase == PHASE_MOVED) game_resolveTile();
            G.screenDirty = true;
        }
    }, 100, nullptr);
}

// =============================================================================
// SCREEN: TILE ACTION
// =============================================================================
static void _evBuyProperty(lv_event_t* e) {
    const Player& p = G.players[G.currentPlayer];
    if (game_buyProperty(G.currentPlayer, p.position)) {
        hw_playCashOut();
    } else {
        hw_playError();
    }
    game_endTurn();
    G.screenDirty = true;
}
static void _evSkipBuy(lv_event_t* e)    { game_endTurn(); G.screenDirty = true; }
static void _evPayRent(lv_event_t* e)    { game_payRent(G.currentPlayer, G.players[G.currentPlayer].position); hw_playCashOut(); game_endTurn(); G.screenDirty = true; }
static void _evContinue(lv_event_t* e)   { game_endTurn(); G.screenDirty = true; }
static void _evPayTax(lv_event_t* e) {
    const Player& p = G.players[G.currentPlayer];
    game_payBank(G.currentPlayer, TILES[p.position].price);
    hw_playCashOut(); game_endTurn(); G.screenDirty = true;
}
static void _evFreeParking(lv_event_t* e) {
    if (G.settings.freeParkingPool && G.freeParkingPool > 0) {
        G.players[G.currentPlayer].money += G.freeParkingPool;
        G.freeParkingPool = 0; hw_playCashIn();
    }
    game_endTurn(); G.screenDirty = true;
}
static void _evGoToJailOk(lv_event_t* e) { game_endTurn(); G.screenDirty = true; }
static void _evBuildHouse(lv_event_t* e) {
    const Player& p = G.players[G.currentPlayer];
    if (game_buildHouse(G.currentPlayer, p.position)) hw_playSuccess();
    else hw_playError();
    G.screenDirty = true;
}

static void _buildTileAction() {
    lv_obj_t* scr = _newScreen();
    const Player& p = G.players[G.currentPlayer];
    const TileData& tile = TILES[p.position];
    lv_color_t gc = (tile.group != GROUP_NONE) ? _groupColor(tile.group) : C_PRIMARY;

    // Tile colour strip at top
    lv_obj_t* strip = lv_obj_create(scr);
    lv_obj_remove_style_all(strip);
    lv_obj_set_size(strip, SCREEN_W, 8);
    lv_obj_set_pos(strip, 0, 0);
    lv_obj_set_style_bg_color(strip, gc, 0);
    lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, 0);

    // Tile name
    _mkLabel(scr, tile.name, LV_ALIGN_TOP_MID, 0, 14, FONT_MD, C_TEXT);

    char buf[48];

    switch (G.tileAction) {
        case ACT_BUY: {
            _mkLabel(scr, "Unowned Property", LV_ALIGN_TOP_MID, 0, 38, FONT_SM, C_TEXT_DIM);
            snprintf(buf, sizeof(buf), "Price: $%d", tile.price);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 58, FONT_MD, C_ACCENT);
            snprintf(buf, sizeof(buf), "Base rent: $%d", tile.rent[0]);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 80, FONT_SM, C_TEXT_DIM);

            bool canBuy = (p.money >= tile.price);
            _mkBtn(scr, "BUY", 30, 105, 120, 45,
                   canBuy ? C_BTN_ACTIVE : lv_color_hex(0x3C3C3C), _evBuyProperty);
            _mkBtn(scr, "SKIP", 170, 105, 120, 45, C_BTN_BG, _evSkipBuy);
            break;
        }
        case ACT_PAY_RENT: {
            int8_t owner = G.props[p.position].owner;
            int32_t rent = game_calcRent(p.position, G.dice1 + G.dice2);
            _mkLabel(scr, "Rent Due!", LV_ALIGN_TOP_MID, 0, 42, FONT_MD, C_DANGER);
            snprintf(buf, sizeof(buf), "Owner: %s", G.players[owner].name);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 66, FONT_SM, C_TEXT);
            snprintf(buf, sizeof(buf), "$%ld", (long)rent);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 88, FONT_XL, C_ACCENT);
            _mkBtn(scr, "PAY RENT", 80, 125, 160, 45, C_DANGER, _evPayRent);
            break;
        }
        case ACT_OWN_PROP: {
            _mkLabel(scr, "Your Property!", LV_ALIGN_TOP_MID, 0, 42, FONT_MD, C_BTN_ACTIVE);
            uint8_t h = G.props[p.position].houses;
            snprintf(buf, sizeof(buf), h == 5 ? "HOTEL" : "Houses: %d", h);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 66, FONT_SM, C_TEXT);

            if (tile.type == TILE_PROPERTY && h < MAX_HOUSES
                && game_ownsFullGroup(G.currentPlayer, tile.group)) {
                snprintf(buf, sizeof(buf), "Upgrade: $%d", tile.houseCost);
                _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 86, FONT_SM, C_ACCENT);
                _mkBtn(scr, "UPGRADE", 20, 108, 130, 40, C_BTN_ACTIVE, _evBuildHouse);
            }
            _mkBtn(scr, "CONTINUE", 170, 108, 130, 40, C_BTN_BG, _evContinue);
            break;
        }
        case ACT_TAX:
            _mkLabel(scr, tile.name, LV_ALIGN_TOP_MID, 0, 50, FONT_MD, C_DANGER);
            snprintf(buf, sizeof(buf), "$%d", tile.price);
            _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 80, FONT_XL, C_ACCENT);
            _mkBtn(scr, "PAY TAX", 80, 120, 160, 45, C_DANGER, _evPayTax);
            break;

        case ACT_FREE_PARKING: {
            _mkLabel(scr, "Free Parking!", LV_ALIGN_TOP_MID, 0, 55, FONT_MD, C_BTN_ACTIVE);
            if (G.settings.freeParkingPool && G.freeParkingPool > 0) {
                snprintf(buf, sizeof(buf), "Collect $%ld!", (long)G.freeParkingPool);
                _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 85, FONT_MD, C_ACCENT);
            } else {
                _mkLabel(scr, "Nothing happens", LV_ALIGN_TOP_MID, 0, 85, FONT_SM, C_TEXT_DIM);
            }
            _mkBtn(scr, "CONTINUE", 80, 115, 160, 45, C_BTN_ACTIVE, _evFreeParking);
            break;
        }
        case ACT_GO_TO_JAIL:
            _mkLabel(scr, "GO TO JAIL!", LV_ALIGN_TOP_MID, 0, 60, FONT_XL, C_DANGER);
            _mkLabel(scr, "Do not pass GO", LV_ALIGN_TOP_MID, 0, 100, FONT_SM, C_TEXT_DIM);
            _mkLabel(scr, "Do not collect $200", LV_ALIGN_TOP_MID, 0, 116, FONT_SM, C_TEXT_DIM);
            _mkBtn(scr, "OK", 80, 145, 160, 40, C_DANGER, _evGoToJailOk);
            hw_playJail();
            break;

        case ACT_JUST_VISITING:
            _mkLabel(scr, "Just Visiting!", LV_ALIGN_TOP_MID, 0, 75, FONT_MD, C_TEXT);
            _mkBtn(scr, "CONTINUE", 80, 115, 160, 40, C_BTN_BG, _evContinue);
            break;

        default:
            _mkLabel(scr, tile.name, LV_ALIGN_TOP_MID, 0, 75, FONT_MD, C_TEXT);
            _mkBtn(scr, "CONTINUE", 80, 115, 160, 40, C_BTN_BG, _evContinue);
            break;
    }

    // Money at bottom
    snprintf(buf, sizeof(buf), "$%ld", (long)p.money);
    _mkLabel(scr, buf, LV_ALIGN_BOTTOM_RIGHT, -5, -5, FONT_MD, C_ACCENT);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: CARD DRAW
// =============================================================================
static void _evCardOk(lv_event_t* e) {
    const CardData& card = G.cardIsChance
        ? CHANCE_CARDS[G.cardIndex]
        : COMMUNITY_CARDS[G.cardIndex];
    hw_playCardDraw();
    game_applyCard(card);

    if (card.effect == CARD_MOVETO || card.effect == CARD_MOVEREL
        || card.effect == CARD_NEAREST_RR || card.effect == CARD_NEAREST_UTIL) {
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
    G.screenDirty = true;
}

static void _buildCardDraw() {
    lv_obj_t* scr = _newScreen();
    const CardData& card = G.cardIsChance
        ? CHANCE_CARDS[G.cardIndex]
        : COMMUNITY_CARDS[G.cardIndex];
    const char* title = G.cardIsChance ? "CHANCE" : "COMMUNITY CHEST";
    lv_color_t titleCol = G.cardIsChance ? C_WARN : lv_color_hex(0x0096DC);

    // Card frame
    lv_obj_t* frame = lv_obj_create(scr);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, 280, 155);
    lv_obj_align(frame, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(frame, lv_color_hex(0xFFFAE6), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(frame, 8, 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, titleCol, 0);
    lv_obj_set_scrollbar_mode(frame, LV_SCROLLBAR_MODE_OFF);

    // Title bar inside card
    lv_obj_t* tbar = lv_obj_create(frame);
    lv_obj_remove_style_all(tbar);
    lv_obj_set_size(tbar, 280, 28);
    lv_obj_set_pos(tbar, 0, 0);
    lv_obj_set_style_bg_color(tbar, titleCol, 0);
    lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, 0);
    lv_obj_t* tl = lv_label_create(tbar);
    lv_label_set_text(tl, title);
    lv_obj_set_style_text_color(tl, C_TEXT, 0);
    lv_obj_set_style_text_font(tl, FONT_MD, 0);
    lv_obj_center(tl);

    // Card text
    lv_obj_t* ct = lv_label_create(frame);
    lv_label_set_text(ct, card.text);
    lv_obj_set_style_text_color(ct, lv_color_black(), 0);
    lv_obj_set_style_text_font(ct, FONT_MD, 0);
    lv_obj_set_width(ct, 260);
    lv_label_set_long_mode(ct, LV_LABEL_LONG_WRAP);
    lv_obj_align(ct, LV_ALIGN_TOP_MID, 0, 38);

    _mkBtn(scr, "OK", 80, 175, 160, 45, C_BTN_ACTIVE, _evCardOk);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: JAIL TURN
// =============================================================================
static void _evJailRoll(lv_event_t* e) {
    bool freed = game_tryJailRoll(G.currentPlayer);
    if (freed) {
        hw_playSuccess();
        game_movePlayer();
        if (G.phase == PHASE_MOVED) game_resolveTile();
    } else {
        hw_playError();
        // Brief delay then end turn
        lv_timer_create([](lv_timer_t* t) {
            game_endTurn();
            G.screenDirty = true;
            lv_timer_delete(t);
        }, 1000, nullptr);
        return;
    }
    G.screenDirty = true;
}
static void _evJailPay(lv_event_t* e) {
    game_payJailFine(G.currentPlayer);
    hw_playCashOut();
    G.phase = PHASE_TURN_START;
    G.screenDirty = true;
}
static void _evJailCard(lv_event_t* e) {
    game_useJailCard(G.currentPlayer);
    hw_playSuccess();
    G.phase = PHASE_TURN_START;
    G.screenDirty = true;
}

static void _buildJailTurn() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "IN JAIL", C_DANGER);

    const Player& p = G.players[G.currentPlayer];
    char buf[40];
    snprintf(buf, sizeof(buf), "%s is in Jail", p.name);
    _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 38, FONT_MD, C_TEXT);
    snprintf(buf, sizeof(buf), "Turn %d of %d", p.jailTurns + 1, G.settings.jailMaxTurns);
    _mkLabel(scr, buf, LV_ALIGN_TOP_MID, 0, 58, FONT_SM, C_TEXT_DIM);

    _mkBtn(scr, "ROLL DOUBLES", 10, 85, 145, 40, C_BTN_BG, _evJailRoll);

    snprintf(buf, sizeof(buf), "PAY $%d", JAIL_FINE);
    _mkBtn(scr, buf, 165, 85, 145, 40, C_DANGER, _evJailPay);

    if (p.hasJailCard) {
        _mkBtn(scr, "USE JAIL-FREE CARD", 10, 135, 300, 35, C_BTN_ACTIVE, _evJailCard);
    }

    _showScreen(scr);
}

// =============================================================================
// SCREEN: TRADE
// =============================================================================
static void _evTradeSelectPlayer(lv_event_t* e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    G.tradeWith = idx;
    G.tradeMoneyOffer = 0;
    G.tradeMoneyRequest = 0;
    G.tradePropsOffer = 0;
    G.tradePropsRequest = 0;
    G.phase = PHASE_TRADE_OFFER;
    G.screenDirty = true;
}

static void _buildTradeSelect() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "Trade - Select Player", C_PRIMARY);

    uint8_t shown = 0;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        if (i == G.currentPlayer || !G.players[i].alive) continue;
        int16_t y = 40 + shown * 38;
        char buf[32];
        snprintf(buf, sizeof(buf), "%s  $%ld", G.players[i].name, (long)G.players[i].money);
        _mkBtn(scr, buf, 20, y, 280, 32, C_BTN_BG,
               _evTradeSelectPlayer, (void*)(uintptr_t)i);
        shown++;
    }

    static GamePhase _phTurn = PHASE_TURN_START;
    _mkBtn(scr, "BACK", 10, 210, 70, 25, C_DANGER, _evBack, &_phTurn);

    _showScreen(scr);
}

// Trade offer
static lv_obj_t* _trOfferLbl = nullptr;
static lv_obj_t* _trRequestLbl = nullptr;

static void _evTradeOfferDec(lv_event_t* e) {
    if (G.tradeMoneyOffer >= 50) G.tradeMoneyOffer -= 50;
    char b[16]; snprintf(b, 16, "Offer: $%ld", (long)G.tradeMoneyOffer);
    lv_label_set_text(_trOfferLbl, b);
}
static void _evTradeOfferInc(lv_event_t* e) {
    G.tradeMoneyOffer += 50;
    char b[16]; snprintf(b, 16, "Offer: $%ld", (long)G.tradeMoneyOffer);
    lv_label_set_text(_trOfferLbl, b);
}
static void _evTradeReqDec(lv_event_t* e) {
    if (G.tradeMoneyRequest >= 50) G.tradeMoneyRequest -= 50;
    char b[16]; snprintf(b, 16, "Request: $%ld", (long)G.tradeMoneyRequest);
    lv_label_set_text(_trRequestLbl, b);
}
static void _evTradeReqInc(lv_event_t* e) {
    G.tradeMoneyRequest += 50;
    char b[16]; snprintf(b, 16, "Request: $%ld", (long)G.tradeMoneyRequest);
    lv_label_set_text(_trRequestLbl, b);
}
static void _evTradeExec(lv_event_t* e) {
    if (game_executeTrade()) hw_playSuccess(); else hw_playError();
    G.phase = PHASE_TURN_START;
    G.screenDirty = true;
}

static void _buildTradeOffer() {
    lv_obj_t* scr = _newScreen();
    const Player& me = G.players[G.currentPlayer];
    const Player& them = G.players[G.tradeWith];
    char buf[40];

    snprintf(buf, sizeof(buf), "Trade with %s", them.name);
    _mkHeader(scr, buf, C_PRIMARY);

    // Money controls
    _trOfferLbl = _mkLabel(scr, "Offer: $0", LV_ALIGN_TOP_LEFT, 10, 34, FONT_SM, _c(me.colour));
    _mkBtn(scr, "-", 10, 50, 30, 22, C_BTN_BG, _evTradeOfferDec);
    _mkBtn(scr, "+", 45, 50, 30, 22, C_BTN_BG, _evTradeOfferInc);

    _trRequestLbl = _mkLabel(scr, "Request: $0", LV_ALIGN_TOP_LEFT, 165, 34, FONT_SM, _c(them.colour));
    _mkBtn(scr, "-", 165, 50, 30, 22, C_BTN_BG, _evTradeReqDec);
    _mkBtn(scr, "+", 200, 50, 30, 22, C_BTN_BG, _evTradeReqInc);

    // Properties (simplified – just show list, toggle with tap)
    _mkLabel(scr, "Props: tap tiles in QuickMenu", LV_ALIGN_TOP_MID, 0, 80, FONT_SM, C_TEXT_DIM);

    _mkBtn(scr, "EXECUTE", 20, 200, 120, 32, C_BTN_ACTIVE, _evTradeExec);
    static GamePhase _phTurn = PHASE_TURN_START;
    _mkBtn(scr, "CANCEL", 160, 200, 120, 32, C_DANGER, _evBack, &_phTurn);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: QUICK MENU
// =============================================================================
static void _evQmBack(lv_event_t* e)    { G.phase = PHASE_TURN_START; G.screenDirty = true; }
static void _evQmEndTurn(lv_event_t* e) { G.isDoubles = false; game_endTurn(); G.screenDirty = true; }
static void _evQmQuit(lv_event_t* e)    { G.phase = PHASE_MENU; G.screenDirty = true; }

static void _buildQuickMenu() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "Quick Menu", C_PRIMARY);

    // All players overview
    int16_t y = 34;
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        const Player& p = G.players[i];
        lv_obj_t* row = lv_obj_create(scr);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 310, 22);
        lv_obj_set_pos(row, 5, y);
        lv_obj_set_style_bg_color(row, (i == G.currentPlayer) ? lv_color_hex(0x1E321E) : C_BG_DARK, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 3, 0);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        // Color dot
        lv_obj_t* dot = lv_obj_create(row);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 10, 10);
        lv_obj_set_pos(dot, 2, 6);
        lv_obj_set_style_bg_color(dot, _c(p.colour), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);

        char buf[48];
        snprintf(buf, sizeof(buf), "%-10s $%-6ld P:%d %s",
                 p.name, (long)p.money, p.position,
                 p.alive ? "" : "(OUT)");
        lv_obj_t* lbl = lv_label_create(row);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, p.alive ? C_TEXT : C_TEXT_DIM, 0);
        lv_obj_set_style_text_font(lbl, FONT_SM, 0);
        lv_obj_set_pos(lbl, 16, 4);

        y += 24;
    }

    // Current player properties
    y += 4;
    _mkLabel(scr, "Your properties:", LV_ALIGN_DEFAULT, 10, y, FONT_SM, C_ACCENT);
    y += 16;
    for (int i = 0; i < BOARD_SIZE && y < 205; i++) {
        if (G.props[i].owner != (int8_t)G.currentPlayer) continue;

        lv_obj_t* pdot = lv_obj_create(scr);
        lv_obj_remove_style_all(pdot);
        lv_obj_set_size(pdot, 6, 10);
        lv_obj_set_pos(pdot, 10, y);
        lv_obj_set_style_bg_color(pdot, _groupColor(TILES[i].group), 0);
        lv_obj_set_style_bg_opa(pdot, LV_OPA_COVER, 0);

        char buf[32];
        uint8_t h = G.props[i].houses;
        snprintf(buf, sizeof(buf), "%s %s%s", TILES[i].name,
                 G.props[i].mortgaged ? "(M)" : "",
                 h == 5 ? " [H]" : h > 0 ? "" : "");
        lv_obj_t* pl = lv_label_create(scr);
        lv_label_set_text(pl, buf);
        lv_obj_set_style_text_color(pl, C_TEXT, 0);
        lv_obj_set_style_text_font(pl, FONT_SM, 0);
        lv_obj_set_pos(pl, 20, y);
        y += 14;
    }

    // Bottom buttons
    _mkBtn(scr, "BACK",     5, 215, 70, 22, C_DANGER, _evQmBack);
    _mkBtn(scr, "END TURN", 80, 215, 80, 22, C_BTN_BG, _evQmEndTurn);
    _mkBtn(scr, "SAVE",     165, 215, 70, 22, C_BTN_BG, _evSaveGame);
    _mkBtn(scr, "QUIT",     240, 215, 70, 22, C_DANGER, _evQmQuit);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: PROGRAMMING MODE
// =============================================================================
static NfcPlayerCard _pCard;

static void _evProgPlayer(lv_event_t* e)   { _progMode = 1; _progStep = 0; memset(&_pCard, 0, sizeof(_pCard)); G.screenDirty = true; }
static void _evProgProperty(lv_event_t* e) { _progMode = 2; _progStep = 0; _propIdx = 1; G.screenDirty = true; }
static void _evProgEvent(lv_event_t* e)    { _progMode = 3; _progStep = 0; G.screenDirty = true; }
static void _evProgBack(lv_event_t* e)     { _progMode = 0; G.screenDirty = true; }

static void _evProgPidDec(lv_event_t* e) { if (_pCard.playerId > 0) _pCard.playerId--; G.screenDirty = true; }
static void _evProgPidInc(lv_event_t* e) { if (_pCard.playerId < 7) _pCard.playerId++; G.screenDirty = true; }
static void _evProgPidNext(lv_event_t* e) { _progStep = 1; G.screenDirty = true; }

static void _evProgPropPrev(lv_event_t* e) {
    do { _propIdx = (_propIdx + BOARD_SIZE - 1) % BOARD_SIZE; }
    while (TILES[_propIdx].type != TILE_PROPERTY && TILES[_propIdx].type != TILE_RAILROAD && TILES[_propIdx].type != TILE_UTILITY);
    G.screenDirty = true;
}
static void _evProgPropNext(lv_event_t* e) {
    do { _propIdx = (_propIdx + 1) % BOARD_SIZE; }
    while (TILES[_propIdx].type != TILE_PROPERTY && TILES[_propIdx].type != TILE_RAILROAD && TILES[_propIdx].type != TILE_UTILITY);
    G.screenDirty = true;
}
static void _evProgPropWrite(lv_event_t* e) { _progStep = 1; G.screenDirty = true; }

static void _buildProgramming() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "Programming Mode", C_WARN);

    if (_progMode == 0) {
        _mkBtn(scr, "Write Player Card", 40, 50, 240, 40, C_BTN_BG, _evProgPlayer);
        _mkBtn(scr, "Write Property Card", 40, 100, 240, 40, C_BTN_BG, _evProgProperty);
        _mkBtn(scr, "Write Event Card", 40, 150, 240, 40, C_BTN_BG, _evProgEvent);
        _mkBtn(scr, "BACK", 10, 210, 70, 25, C_DANGER, _evBack, &_phMenu);
    }
    else if (_progMode == 1) {
        if (_progStep == 0) {
            _mkLabel(scr, "Player ID (0-7):", LV_ALIGN_TOP_MID, 0, 45, FONT_MD, C_TEXT);
            char b[4]; snprintf(b, 4, "%d", _pCard.playerId);
            _mkLabel(scr, b, LV_ALIGN_TOP_MID, 0, 75, FONT_XL, C_ACCENT);
            _mkBtn(scr, "-", 60, 100, 40, 30, C_BTN_BG, _evProgPidDec);
            _mkBtn(scr, "+", 220, 100, 40, 30, C_BTN_BG, _evProgPidInc);
            _mkBtn(scr, "NEXT", 80, 150, 160, 35, C_BTN_ACTIVE, _evProgPidNext);
        } else if (_progStep == 1) {
            _mkLabel(scr, "Enter name then scan card", LV_ALIGN_TOP_MID, 0, 50, FONT_MD, C_TEXT);
            _pCard.colour = PLAYER_COLOURS[_pCard.playerId];
            memset(_pCard.name, 0, sizeof(_pCard.name));
            // Launch keyboard (blocking)
            _showScreen(scr);
            ui_keyboard(_pCard.name, MAX_NAME_LEN, "Player Name");
            _progStep = 2;
            G.screenDirty = true;
            return;
        } else if (_progStep == 2) {
            _mkLabel(scr, "SCAN CARD NOW", LV_ALIGN_TOP_MID, 0, 60, FONT_MD, C_ACCENT);
            char info[40]; snprintf(info, sizeof(info), "ID:%d Name:%s", _pCard.playerId, _pCard.name);
            _mkLabel(scr, info, LV_ALIGN_TOP_MID, 0, 90, FONT_SM, C_TEXT);
            _mkBtn(scr, "CANCEL", 10, 210, 70, 25, C_DANGER, _evProgBack);

            // NFC poll timer
            _showScreen(scr);
            _activeTimer = lv_timer_create([](lv_timer_t* t) {
                uint8_t uid[7]; uint8_t uidLen;
                if (nfc_pollCard(uid, &uidLen, 100)) {
                    _pCard.type = NFC_TYPE_PLAYER;
                    if (nfc_writePlayerCard(uid, uidLen, _pCard)) hw_playSuccess();
                    else hw_playError();
                    _progMode = 0;
                    G.phase = PHASE_PROGRAMMING;
                    G.screenDirty = true;
                }
            }, NFC_POLL_INTERVAL_MS, nullptr);
            return;
        }
    }
    else if (_progMode == 2) {
        if (_progStep == 0) {
            _mkLabel(scr, "Select Property:", LV_ALIGN_TOP_MID, 0, 40, FONT_MD, C_TEXT);
            char b[32]; snprintf(b, sizeof(b), "#%d: %s", _propIdx, TILES[_propIdx].name);
            _mkLabel(scr, b, LV_ALIGN_TOP_MID, 0, 68, FONT_MD, C_ACCENT);

            lv_obj_t* cs = lv_obj_create(scr);
            lv_obj_remove_style_all(cs);
            lv_obj_set_size(cs, 200, 6);
            lv_obj_align(cs, LV_ALIGN_TOP_MID, 0, 92);
            lv_obj_set_style_bg_color(cs, _groupColor(TILES[_propIdx].group), 0);
            lv_obj_set_style_bg_opa(cs, LV_OPA_COVER, 0);

            _mkBtn(scr, "PREV", 20, 108, 60, 35, C_BTN_BG, _evProgPropPrev);
            _mkBtn(scr, "NEXT", 240, 108, 60, 35, C_BTN_BG, _evProgPropNext);
            _mkBtn(scr, "WRITE", 100, 155, 120, 35, C_BTN_ACTIVE, _evProgPropWrite);
        } else if (_progStep == 1) {
            _mkLabel(scr, "SCAN CARD NOW", LV_ALIGN_TOP_MID, 0, 60, FONT_MD, C_ACCENT);
            _mkLabel(scr, TILES[_propIdx].name, LV_ALIGN_TOP_MID, 0, 90, FONT_MD, C_TEXT);
            _mkBtn(scr, "CANCEL", 10, 210, 70, 25, C_DANGER, _evProgBack);

            _showScreen(scr);
            _activeTimer = lv_timer_create([](lv_timer_t* t) {
                uint8_t uid[7]; uint8_t uidLen;
                if (nfc_pollCard(uid, &uidLen, 100)) {
                    NfcPropertyCard card;
                    card.type = NFC_TYPE_PROPERTY;
                    card.tileIndex = _propIdx;
                    card.group = TILES[_propIdx].group;
                    strncpy(card.name, TILES[_propIdx].name, MAX_NAME_LEN);
                    if (nfc_writePropertyCard(uid, uidLen, card)) hw_playSuccess();
                    else hw_playError();
                    _progMode = 0; _progStep = 0;
                    G.phase = PHASE_PROGRAMMING;
                    G.screenDirty = true;
                }
            }, NFC_POLL_INTERVAL_MS, nullptr);
            return;
        }
    }
    else if (_progMode == 3) {
        _mkLabel(scr, "SCAN EVENT CARD", LV_ALIGN_TOP_MID, 0, 65, FONT_MD, C_ACCENT);
        _mkLabel(scr, "Card will be written as", LV_ALIGN_TOP_MID, 0, 95, FONT_SM, C_TEXT_DIM);
        _mkLabel(scr, "generic event token", LV_ALIGN_TOP_MID, 0, 112, FONT_SM, C_TEXT_DIM);
        _mkBtn(scr, "CANCEL", 10, 210, 70, 25, C_DANGER, _evProgBack);

        _showScreen(scr);
        _activeTimer = lv_timer_create([](lv_timer_t* t) {
            uint8_t uid[7]; uint8_t uidLen;
            if (nfc_pollCard(uid, &uidLen, 100)) {
                NfcEventCard card;
                card.type = NFC_TYPE_EVENT;
                card.eventId = 0;
                if (nfc_writeEventCard(uid, uidLen, card)) hw_playSuccess();
                else hw_playError();
                _progMode = 0;
                G.phase = PHASE_PROGRAMMING;
                G.screenDirty = true;
            }
        }, NFC_POLL_INTERVAL_MS, nullptr);
        return;
    }

    _showScreen(scr);
}

// =============================================================================
// SCREEN: SETTINGS
// =============================================================================
static void _evSettSave(lv_event_t* e)  { storage_saveSettings(G.settings); hw_setVolume(G.settings.volume); hw_playSuccess(); }

static void _buildSettings() {
    lv_obj_t* scr = _newScreen();
    _mkHeader(scr, "Settings", lv_color_hex(0x505078));

    GameSettings& s = G.settings;
    int16_t y = 36;
    char buf[40];

    auto addRow = [&](const char* label, const char* val) {
        lv_obj_t* row = lv_obj_create(scr);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, SCREEN_W, 22);
        lv_obj_set_pos(row, 0, y);
        lv_obj_set_style_bg_color(row, C_BG_DARK, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_60, 0);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t* ll = lv_label_create(row);
        lv_label_set_text(ll, label);
        lv_obj_set_style_text_color(ll, C_TEXT, 0);
        lv_obj_set_style_text_font(ll, FONT_SM, 0);
        lv_obj_set_pos(ll, 10, 3);

        lv_obj_t* vl = lv_label_create(row);
        lv_label_set_text(vl, val);
        lv_obj_set_style_text_color(vl, C_ACCENT, 0);
        lv_obj_set_style_text_font(vl, FONT_SM, 0);
        lv_obj_align(vl, LV_ALIGN_RIGHT_MID, -10, 0);

        // Make row clickable
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_group_add_obj(lv_group_get_default(), row);
        lv_obj_set_style_outline_width(row, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(row, C_ACCENT, LV_STATE_FOCUSED);

        y += 24;
        return row;
    };

    snprintf(buf, sizeof(buf), "$%lu", (unsigned long)s.startingMoney);
    lv_obj_t* r0 = addRow("Starting Money", buf);
    lv_obj_add_event_cb(r0, [](lv_event_t* e) {
        GameSettings& s = G.settings;
        s.startingMoney = (s.startingMoney == 1500) ? 2000 : (s.startingMoney == 2000) ? 2500 : (s.startingMoney == 2500) ? 1000 : 1500;
        G.screenDirty = true;
    }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* r1 = addRow("Free Parking Pool", s.freeParkingPool ? "ON" : "OFF");
    lv_obj_add_event_cb(r1, [](lv_event_t* e) { G.settings.freeParkingPool = !G.settings.freeParkingPool; G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    snprintf(buf, sizeof(buf), "%d turns", s.jailMaxTurns);
    lv_obj_t* r2 = addRow("Jail Max Turns", buf);
    lv_obj_add_event_cb(r2, [](lv_event_t* e) { G.settings.jailMaxTurns = (G.settings.jailMaxTurns % 5) + 1; G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* r3 = addRow("Auto Rent", s.autoRent ? "ON" : "OFF");
    lv_obj_add_event_cb(r3, [](lv_event_t* e) { G.settings.autoRent = !G.settings.autoRent; G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* r4 = addRow("Require NFC", s.nfcRequired ? "ON" : "OFF");
    lv_obj_add_event_cb(r4, [](lv_event_t* e) { G.settings.nfcRequired = !G.settings.nfcRequired; G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    snprintf(buf, sizeof(buf), "%d", s.diceSpeed);
    lv_obj_t* r5 = addRow("Dice Speed (1-3)", buf);
    lv_obj_add_event_cb(r5, [](lv_event_t* e) { G.settings.diceSpeed = (G.settings.diceSpeed % 3) + 1; G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    snprintf(buf, sizeof(buf), "%d", s.volume);
    lv_obj_t* r6 = addRow("Volume (0-5)", buf);
    lv_obj_add_event_cb(r6, [](lv_event_t* e) { G.settings.volume = (G.settings.volume + 1) % 6; hw_setVolume(G.settings.volume); G.screenDirty = true; }, LV_EVENT_CLICKED, nullptr);

    _mkBtn(scr, "SAVE", 20, 210, 120, 25, C_BTN_ACTIVE, _evSettSave);
    _mkBtn(scr, "BACK", 160, 210, 120, 25, C_DANGER, _evBack, &_phMenu);

    _showScreen(scr);
}

// =============================================================================
// SCREEN: GAME OVER
// =============================================================================
static void _evGameOverMenu(lv_event_t* e) {
    storage_clearSave();
    G.phase = PHASE_MENU;
    G.screenDirty = true;
}

static void _buildGameOver() {
    lv_obj_t* scr = _newScreen();
    uint8_t w = game_getWinner();
    const Player& p = G.players[w];

    _mkLabel(scr, "GAME OVER!", LV_ALIGN_TOP_MID, 0, 30, FONT_XL, C_ACCENT);

    // Winner colour circle
    lv_obj_t* circ = lv_obj_create(scr);
    lv_obj_remove_style_all(circ);
    lv_obj_set_size(circ, 40, 40);
    lv_obj_align(circ, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(circ, _c(p.colour), 0);
    lv_obj_set_style_bg_opa(circ, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(circ, LV_RADIUS_CIRCLE, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "%s WINS!", p.name);
    _mkLabel(scr, buf, LV_ALIGN_CENTER, 0, 20, FONT_MD, C_TEXT);
    snprintf(buf, sizeof(buf), "Final: $%ld", (long)p.money);
    _mkLabel(scr, buf, LV_ALIGN_CENTER, 0, 42, FONT_SM, C_TEXT_DIM);

    _mkBtn(scr, "MAIN MENU", 80, 190, 160, 40, C_BTN_ACTIVE, _evGameOverMenu);

    _showScreen(scr);
}

// =============================================================================
// ON-SCREEN KEYBOARD (blocking)
// =============================================================================
static volatile bool _kbDone = false;

bool ui_keyboard(char* buf, uint8_t maxLen, const char* prompt) {
    lv_obj_t* scr = _newScreen();
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Prompt
    _mkLabel(scr, prompt, LV_ALIGN_TOP_MID, 0, 5, FONT_MD, C_TEXT);

    // Text area
    lv_obj_t* ta = lv_textarea_create(scr);
    lv_textarea_set_max_length(ta, maxLen);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_size(ta, 300, 36);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_text_color(ta, C_TEXT, 0);
    lv_obj_set_style_text_font(ta, FONT_MD, 0);
    if (strlen(buf) > 0) lv_textarea_set_text(ta, buf);

    // Keyboard
    lv_obj_t* kb = lv_keyboard_create(scr);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, SCREEN_W, 160);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    _kbDone = false;

    // Ready callback – fires on LV_KEY_ENTER from keyboard
    lv_obj_add_event_cb(kb, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
            _kbDone = true;
        }
    }, LV_EVENT_ALL, nullptr);

    lv_screen_load(scr);

    // Blocking loop
    while (!_kbDone) {
        lv_timer_handler();
        hw_updateAudio();
        BtnId btn = hw_readButtons();
        if (btn == BTN_CENTER) _kbDone = true;
        delay(5);
    }

    // Copy result
    const char* result = lv_textarea_get_text(ta);
    strncpy(buf, result, maxLen);
    buf[maxLen] = '\0';

    lv_obj_delete(scr);
    return strlen(buf) > 0;
}

// =============================================================================
// MASTER UI INIT / UPDATE
// =============================================================================
void ui_init() {
    _initColors();
    _prevPhase = (GamePhase)0xFF;
    DBG_PRINT("UI init done (LVGL)");
}

void ui_update() {
    // React to game state changes
    if (G.phase != _prevPhase || G.screenDirty) {
        GamePhase ph = G.phase;
        _prevPhase = ph;
        G.screenDirty = false;

        // Auto-resolve PHASE_MOVED (no screen needed)
        if (ph == PHASE_MOVED) {
            game_resolveTile();
            ph = G.phase;
            _prevPhase = ph;
        }

        DBG("UI: phase -> %d", (int)ph);

        switch (ph) {
            case PHASE_SPLASH:         _buildSplash();       break;
            case PHASE_MENU:           _buildMenu();         break;
            case PHASE_SETUP_COUNT:    _buildSetupCount();   break;
            case PHASE_SETUP_PLAYERS:  _buildSetupPlayers(); break;
            case PHASE_TURN_START:     _buildTurnStart();    break;
            case PHASE_ROLLING:        _buildRolling();      break;
            case PHASE_TILE_ACTION:    _buildTileAction();   break;
            case PHASE_CARD_DRAW:      _buildCardDraw();     break;
            case PHASE_JAIL_TURN:      _buildJailTurn();     break;
            case PHASE_TRADE_SELECT:   _buildTradeSelect();  break;
            case PHASE_TRADE_OFFER:    _buildTradeOffer();   break;
            case PHASE_QUICK_MENU:     _buildQuickMenu();    break;
            case PHASE_PROGRAMMING:    _buildProgramming();  break;
            case PHASE_SETTINGS:       _buildSettings();     break;
            case PHASE_GAME_OVER:      _buildGameOver();     break;
            default: break;
        }
    }
}
