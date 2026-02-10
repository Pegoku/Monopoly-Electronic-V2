#include "hardware.h"

// =============================================================================
// GLOBALS
// =============================================================================
TFT_eSPI tft = TFT_eSPI();

// =============================================================================
// DISPLAY
// =============================================================================
void hw_initDisplay() {
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);           // landscape
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void hw_backlight(bool on) {
    digitalWrite(PIN_TFT_BL, on ? HIGH : LOW);
}

// =============================================================================
// TOUCH  (XPT2046 via TFT_eSPI built-in driver)
// =============================================================================
static uint16_t _calData[5] = {300, 3600, 300, 3600, 3};
static uint32_t _lastTouchMs = 0;

void hw_initTouch() {
    tft.setTouch(_calData);
    pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
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
