#pragma once
#include <Arduino.h>

// =============================================================================
// PIN DEFINITIONS
// =============================================================================

// Display (TFT_eSPI handles SPI pins via build_flags)
#define PIN_TFT_BL      21

// Touch
#define PIN_TOUCH_IRQ    18

// NFC PN532 (I2C)
#define PIN_NFC_SDA      8
#define PIN_NFC_SCL      9
#define PIN_NFC_IRQ      -1
#define PIN_NFC_RST      -1

// Physical buttons (active LOW, use INPUT_PULLUP)
#define PIN_BTN_LEFT     5
#define PIN_BTN_CENTER   6
#define PIN_BTN_RIGHT    7

// Speaker / buzzer
#define PIN_SPEAKER      4

// =============================================================================
// DISPLAY
// =============================================================================
#define SCREEN_W         320
#define SCREEN_H         240

// =============================================================================
// GAME CONSTANTS
// =============================================================================
#define MAX_PLAYERS          8
#define MAX_NAME_LEN         12
#define STARTING_MONEY       1500
#define GO_SALARY            200
#define JAIL_POSITION        10
#define GO_TO_JAIL_POS       30
#define JAIL_FINE            50
#define INCOME_TAX_AMT       200
#define LUXURY_TAX_AMT       100
#define BOARD_SIZE           40
#define MAX_HOUSES           5      // 5 = hotel
#define TOTAL_HOUSES         32     // Bank's supply
#define TOTAL_HOTELS         12
#define NUM_CHANCE_CARDS     16
#define NUM_COMMUNITY_CARDS  16

// =============================================================================
// NFC CARD TYPES  (byte 0 of sector 1 block 4)
// =============================================================================
#define NFC_TYPE_PLAYER      0x01
#define NFC_TYPE_PROPERTY    0x02
#define NFC_TYPE_EVENT       0x03

// =============================================================================
// TIMING
// =============================================================================
#define BTN_DEBOUNCE_MS      50
#define TOUCH_DEBOUNCE_MS    100
#define SPLASH_DURATION_MS   2500
#define DICE_ANIM_MS         1200
#define NFC_POLL_INTERVAL_MS 300

// =============================================================================
// RGB565 COLOUR HELPER
// =============================================================================
#define RGB565(r, g, b) (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// =============================================================================
// THEME COLOURS  (RGB565)
// =============================================================================
#define COL_BG              0x0000   // Black
#define COL_BG_DARK         0x0841   // Very dark grey
#define COL_PRIMARY         RGB565(0, 100, 60)   // Monopoly green
#define COL_ACCENT          RGB565(255, 200, 0)  // Gold
#define COL_TEXT            0xFFFF   // White
#define COL_TEXT_DIM        0x7BEF   // Mid grey
#define COL_TEXT_DARK       0x2945   // Dark grey text
#define COL_BTN_BG          RGB565(40, 40, 50)
#define COL_BTN_ACTIVE      RGB565(0, 180, 80)
#define COL_HEADER          RGB565(20, 20, 35)
#define COL_DANGER          RGB565(220, 40, 40)
#define COL_WARN            RGB565(240, 180, 0)

// Property-group colours
#define COL_GRP_BROWN       RGB565(139, 69, 19)
#define COL_GRP_LTBLUE      RGB565(135, 206, 235)
#define COL_GRP_PINK        RGB565(216, 27, 96)
#define COL_GRP_ORANGE      RGB565(255, 140, 0)
#define COL_GRP_RED         RGB565(255, 0, 0)
#define COL_GRP_YELLOW      RGB565(255, 255, 0)
#define COL_GRP_GREEN       RGB565(0, 166, 81)
#define COL_GRP_DKBLUE      RGB565(0, 0, 200)
#define COL_GRP_RR          RGB565(80, 80, 80)
#define COL_GRP_UTIL        RGB565(200, 200, 200)

// Player colours (up to 8)
const uint16_t PLAYER_COLOURS[MAX_PLAYERS] = {
    RGB565(220, 40, 40),   // Red
    RGB565(40, 100, 220),  // Blue
    RGB565(40, 180, 60),   // Green
    RGB565(240, 220, 0),   // Yellow
    RGB565(160, 40, 200),  // Purple
    RGB565(255, 140, 0),   // Orange
    RGB565(0, 200, 200),   // Cyan
    RGB565(255, 100, 180), // Pink
};

// =============================================================================
// GAME SETTINGS (run-time adjustable)
// =============================================================================
struct GameSettings {
    uint32_t startingMoney   = STARTING_MONEY;
    bool     freeParkingPool = true;
    uint8_t  jailMaxTurns    = 3;
    bool     autoRent        = true;
    bool     nfcRequired     = false;
    uint8_t  diceSpeed       = 2;        // 1-3
    uint8_t  volume          = 3;        // 0-5
};
