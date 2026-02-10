#include "hardware.h"
#include <lvgl.h>

// =============================================================================
// GLOBALS
// =============================================================================
TFT_eSPI tft = TFT_eSPI();

// =============================================================================
// DISPLAY
// =============================================================================
void hw_initDisplay() {
    DBG_PRINT("Display init start");
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);           // landscape
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    DBG("Display init done: %dx%d rotation=%d", tft.width(), tft.height(), tft.getRotation());
}

void hw_backlight(bool on) {
    digitalWrite(PIN_TFT_BL, on ? HIGH : LOW);
}

// =============================================================================
// TOUCH  (XPT2046 via TFT_eSPI built-in driver)
// =============================================================================
static uint32_t _lastTouchMs = 0;

void hw_initTouch() {
    uint16_t calData[5] = {300, 3600, 300, 3600, 1};   // last value must match tft.setRotation()
    tft.setTouch(calData);
    pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
    DBG_PRINT("Touch init done");
}

TouchPoint hw_getTouch() {
    TouchPoint tp;
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty, 40)) {
        if (millis() - _lastTouchMs > TOUCH_DEBOUNCE_MS) {
            tp.x = tx;
            tp.y = ty;
            tp.pressed = true;
            _lastTouchMs = millis();
        }
    }
    return tp;
}

bool hw_touchInRect(int16_t tx, int16_t ty,
                    int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
    return tx >= rx && tx < rx + rw && ty >= ry && ty < ry + rh;
}

// =============================================================================
// BUTTONS (debounced, edge detection)
// =============================================================================
static const uint8_t _btnPins[3] = {PIN_BTN_LEFT, PIN_BTN_CENTER, PIN_BTN_RIGHT};
static bool  _btnPrev[3]     = {false, false, false};
static uint32_t _btnTime[3]  = {0, 0, 0};
static bool  _btnDown[3]     = {false, false, false};

void hw_initButtons() {
    for (int i = 0; i < 3; i++) {
        pinMode(_btnPins[i], INPUT_PULLUP);
    }
    DBG_PRINT("Buttons init done");
}

BtnId hw_readButtons() {
    for (int i = 0; i < 3; i++) {
        bool cur = (digitalRead(_btnPins[i]) == LOW);
        _btnDown[i] = cur;
        if (cur && !_btnPrev[i] && (millis() - _btnTime[i] > BTN_DEBOUNCE_MS)) {
            _btnPrev[i] = true;
            _btnTime[i] = millis();
            return (BtnId)(i + 1);   // BTN_LEFT=1, BTN_CENTER=2, BTN_RIGHT=3
        }
        if (!cur) _btnPrev[i] = false;
    }
    return BTN_NONE;
}

bool hw_isBtnHeld(BtnId b, uint32_t ms) {
    if (b == BTN_NONE || b > BTN_RIGHT) return false;
    uint8_t idx = b - 1;
    return _btnDown[idx] && (millis() - _btnTime[idx] >= ms);
}

// =============================================================================
// AUDIO  (non-blocking tone sequencer)
// =============================================================================
static const Note* _melody  = nullptr;
static uint8_t     _melLen  = 0;
static uint8_t     _melIdx  = 0;
static uint32_t    _noteStart = 0;
static uint8_t     _volume    = 3;   // 0-5

void hw_initAudio() {
    pinMode(PIN_SPEAKER, OUTPUT);
    noTone(PIN_SPEAKER);
    DBG_PRINT("Audio init done");
}

void hw_setVolume(uint8_t vol) {
    _volume = vol > 5 ? 5 : vol;
    if (_volume == 0) {
        noTone(PIN_SPEAKER);
        _melody = nullptr;
    }
}

void hw_playMelody(const Note* melody, uint8_t len) {
    if (_volume == 0) return;
    _melody   = melody;
    _melLen   = len;
    _melIdx   = 0;
    _noteStart = millis();
    if (_melody[0].freq > 0) tone(PIN_SPEAKER, _melody[0].freq);
    else                     noTone(PIN_SPEAKER);
}

void hw_updateAudio() {
    if (!_melody) return;
    if (millis() - _noteStart >= _melody[_melIdx].durationMs) {
        _melIdx++;
        if (_melIdx >= _melLen) {
            _melody = nullptr;
            noTone(PIN_SPEAKER);
            return;
        }
        _noteStart = millis();
        if (_melody[_melIdx].freq > 0)
            tone(PIN_SPEAKER, _melody[_melIdx].freq);
        else
            noTone(PIN_SPEAKER);
    }
}

// ---- Pre-built melodies ----

static const Note _jingle[] = {
    {523,120},{659,120},{784,120},{1047,250},{0,80},{784,120},{1047,350}
};
void hw_playJingle() { hw_playMelody(_jingle, 7); }

static const Note _dice[] = {
    {200,40},{400,40},{300,40},{500,40},{250,40},{450,40},{350,40},{600,80}
};
void hw_playDiceRoll() { hw_playMelody(_dice, 8); }

static const Note _cashIn[] = {
    {800,80},{1000,80},{1200,120}
};
void hw_playCashIn() { hw_playMelody(_cashIn, 3); }

static const Note _cashOut[] = {
    {600,80},{400,80},{300,120}
};
void hw_playCashOut() { hw_playMelody(_cashOut, 3); }

static const Note _cardDraw[] = {
    {400,100},{500,100},{600,100},{700,200}
};
void hw_playCardDraw() { hw_playMelody(_cardDraw, 4); }

static const Note _error[] = {
    {200,180},{150,250}
};
void hw_playError()   { hw_playMelody(_error, 2); }

static const Note _success[] = {
    {523,80},{659,80},{784,80},{1047,180}
};
void hw_playSuccess() { hw_playMelody(_success, 4); }

static const Note _jail[] = {
    {400,150},{300,150},{200,300}
};
void hw_playJail()    { hw_playMelody(_jail, 3); }

// =============================================================================
// LVGL DISPLAY + INPUT DRIVERS
// =============================================================================

// Draw buffer (1/10 of screen, 2 bytes per pixel for RGB565)
#define DRAW_BUF_LINES 24
static uint8_t _lvBuf[SCREEN_W * DRAW_BUF_LINES * sizeof(lv_color16_t)];

// Display flush callback: push rendered pixels to TFT via SPI
static void _lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)px_map, w * h, true);   // true = swap bytes for SPI
    tft.endWrite();
    lv_display_flush_ready(disp);
}

// Touch input read callback
static uint16_t _calData[5] = {300, 3600, 300, 3600, 3};

static void _lvgl_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty, 40)) {
        data->point.x = tx;
        data->point.y = ty;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Keypad (3 physical buttons) read callback
static uint32_t _lastKey = 0;

static void _lvgl_keypad_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    BtnId btn = hw_readButtons();
    if (btn != BTN_NONE) {
        switch (btn) {
            case BTN_LEFT:   _lastKey = LV_KEY_LEFT;  break;
            case BTN_CENTER: _lastKey = LV_KEY_ENTER; break;
            case BTN_RIGHT:  _lastKey = LV_KEY_RIGHT; break;
            default: break;
        }
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->key = _lastKey;
}

// LVGL tick source (using Arduino millis)
static uint32_t _lvgl_tick_cb(void) {
    return millis();
}

// LVGL group for button navigation
static lv_group_t* _defaultGroup = nullptr;

void hw_lvgl_init() {
    DBG_PRINT("LVGL init start");

    lv_init();
    lv_tick_set_cb(_lvgl_tick_cb);

    // --- Display driver ---
    lv_display_t* disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_buffers(disp, _lvBuf, NULL, sizeof(_lvBuf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, _lvgl_flush_cb);
    DBG("LVGL display: %dx%d, buf=%u bytes", SCREEN_W, SCREEN_H, (unsigned)sizeof(_lvBuf));

    // --- Touch input device ---
    lv_indev_t* touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, _lvgl_touch_read_cb);

    // --- Keypad input device (3 buttons) ---
    lv_indev_t* keypad_indev = lv_indev_create();
    lv_indev_set_type(keypad_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad_indev, _lvgl_keypad_read_cb);

    // --- Default group for button navigation ---
    _defaultGroup = lv_group_create();
    lv_group_set_default(_defaultGroup);
    lv_indev_set_group(keypad_indev, _defaultGroup);

    DBG_PRINT("LVGL init done");
}
