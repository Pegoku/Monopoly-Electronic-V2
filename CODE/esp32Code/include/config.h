#pragma once
#include <Arduino.h>

// =============================================================================
// DEBUG
// =============================================================================
#ifndef DEBUG
  #define DEBUG 0
#endif

#if DEBUG
  #define DBG(fmt, ...)   Serial.printf("[DBG] " fmt "\n", ##__VA_ARGS__)
  #define DBG_PRINT(msg)  Serial.println(F("[DBG] " msg))
#else
  #define DBG(fmt, ...)   ((void)0)
  #define DBG_PRINT(msg)  ((void)0)
#endif

// =============================================================================
// PIN DEFINITIONS
// =============================================================================

// Display pins (ST7789 SPI)
#define PIN_TFT_CS      14
#define PIN_TFT_DC      16
#define PIN_TFT_RST     38
#define PIN_TFT_SCLK    12
#define PIN_TFT_MOSI    11
#define PIN_TFT_MISO    13
#define PIN_TFT_BL      21

// Touch
#define PIN_TOUCH_IRQ    18

// NFC PN532 (I2C)
#define PIN_NFC_SDA      8
#define PIN_NFC_SCL      9
#define PIN_NFC_IRQ      -1
#define PIN_NFC_RST      -1

// Physical buttons (active LOW, use INPUT_PULLUP)
#define PIN_BTN_LEFT     3
#define PIN_BTN_CENTER   10
#define PIN_BTN_RIGHT    46

// Speaker / buzzer
#define PIN_SPEAKER      4

// Battery ADC
#define PIN_BATTERY_ADC  1

// Button aliases used by current firmware modules
#define PIN_BTN1         PIN_BTN_LEFT
#define PIN_BTN2         PIN_BTN_CENTER
#define PIN_BTN3         PIN_BTN_RIGHT

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

// Timing aliases used by current firmware modules
#define NFC_POLL_MS       140
#define CARD_DEBOUNCE_MS  1200
#define WAIT_TIMEOUT_MS   20000
#define HOME_REFRESH_MS   500


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
