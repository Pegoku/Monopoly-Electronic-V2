#pragma once
#include <Arduino.h>

// =============================================================================
// TILE TYPES
// =============================================================================
enum TileType : uint8_t {
    TILE_GO = 0,
    TILE_PROPERTY,
    TILE_RAILROAD,
    TILE_UTILITY,
    TILE_CHANCE,
    TILE_COMMUNITY,
    TILE_TAX,
    TILE_JAIL,
    TILE_FREE_PARKING,
    TILE_GO_TO_JAIL
};

// =============================================================================
// COLOUR GROUPS
// =============================================================================
enum ColorGroup : uint8_t {
    GROUP_NONE = 0,
    GROUP_BROWN,        // 2 properties
    GROUP_LIGHT_BLUE,   // 3
    GROUP_PINK,         // 3
    GROUP_ORANGE,       // 3
    GROUP_RED,          // 3
    GROUP_YELLOW,       // 3
    GROUP_GREEN,        // 3
    GROUP_DARK_BLUE,    // 2
    GROUP_RAILROAD,     // 4
    GROUP_UTILITY,      // 2
    NUM_GROUPS
};

// How many properties per group (index = ColorGroup)
const uint8_t GROUP_SIZE[] = {0, 2, 3, 3, 3, 3, 3, 3, 2, 4, 2};

// =============================================================================
// BOARD TILE DATA
// =============================================================================
struct TileData {
    const char* name;
    TileType   type;
    ColorGroup group;
    uint16_t   price;
    uint16_t   rent[6];      // base, 1-4 houses, hotel
    uint16_t   houseCost;
    uint16_t   mortgage;
};

const TileData TILES[40] = {
    /*  0 */ {"GO",                TILE_GO,           GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /*  1 */ {"Mediterranean Ave", TILE_PROPERTY,     GROUP_BROWN,     60, {  2, 10, 30,  90, 160, 250},  50,  30},
    /*  2 */ {"Community Chest",   TILE_COMMUNITY,    GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /*  3 */ {"Baltic Ave",        TILE_PROPERTY,     GROUP_BROWN,     60, {  4, 20, 60, 180, 320, 450},  50,  30},
    /*  4 */ {"Income Tax",        TILE_TAX,          GROUP_NONE,     200, {  0,  0,  0,   0,   0,   0},   0,   0},
    /*  5 */ {"Reading Railroad",  TILE_RAILROAD,     GROUP_RAILROAD, 200, { 25, 50,100, 200,   0,   0},   0, 100},
    /*  6 */ {"Oriental Ave",      TILE_PROPERTY,     GROUP_LIGHT_BLUE,100,{  6, 30, 90, 270, 400, 550},  50,  50},
    /*  7 */ {"Chance",            TILE_CHANCE,       GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /*  8 */ {"Vermont Ave",       TILE_PROPERTY,     GROUP_LIGHT_BLUE,100,{  6, 30, 90, 270, 400, 550},  50,  50},
    /*  9 */ {"Connecticut Ave",   TILE_PROPERTY,     GROUP_LIGHT_BLUE,120,{  8, 40,100, 300, 450, 600},  50,  60},
    /* 10 */ {"Jail",              TILE_JAIL,         GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 11 */ {"St. Charles Pl",    TILE_PROPERTY,     GROUP_PINK,     140, { 10, 50,150, 450, 625, 750}, 100,  70},
    /* 12 */ {"Electric Company",  TILE_UTILITY,      GROUP_UTILITY,  150, {  0,  0,  0,   0,   0,   0},   0,  75},
    /* 13 */ {"States Ave",        TILE_PROPERTY,     GROUP_PINK,     140, { 10, 50,150, 450, 625, 750}, 100,  70},
    /* 14 */ {"Virginia Ave",      TILE_PROPERTY,     GROUP_PINK,     160, { 12, 60,180, 500, 700, 900}, 100,  80},
    /* 15 */ {"Pennsylvania RR",   TILE_RAILROAD,     GROUP_RAILROAD, 200, { 25, 50,100, 200,   0,   0},   0, 100},
    /* 16 */ {"St. James Pl",      TILE_PROPERTY,     GROUP_ORANGE,   180, { 14, 70,200, 550, 750, 950}, 100,  90},
    /* 17 */ {"Community Chest",   TILE_COMMUNITY,    GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 18 */ {"Tennessee Ave",     TILE_PROPERTY,     GROUP_ORANGE,   180, { 14, 70,200, 550, 750, 950}, 100,  90},
    /* 19 */ {"New York Ave",      TILE_PROPERTY,     GROUP_ORANGE,   200, { 16, 80,220, 600, 800,1000}, 100, 100},
    /* 20 */ {"Free Parking",      TILE_FREE_PARKING, GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 21 */ {"Kentucky Ave",      TILE_PROPERTY,     GROUP_RED,      220, { 18, 90,250, 700, 875,1050}, 150, 110},
    /* 22 */ {"Chance",            TILE_CHANCE,       GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 23 */ {"Indiana Ave",       TILE_PROPERTY,     GROUP_RED,      220, { 18, 90,250, 700, 875,1050}, 150, 110},
    /* 24 */ {"Illinois Ave",      TILE_PROPERTY,     GROUP_RED,      240, { 20,100,300, 750, 925,1100}, 150, 120},
    /* 25 */ {"B&O Railroad",      TILE_RAILROAD,     GROUP_RAILROAD, 200, { 25, 50,100, 200,   0,   0},   0, 100},
    /* 26 */ {"Atlantic Ave",      TILE_PROPERTY,     GROUP_YELLOW,   260, { 22,110,330, 800, 975,1150}, 150, 130},
    /* 27 */ {"Ventnor Ave",       TILE_PROPERTY,     GROUP_YELLOW,   260, { 22,110,330, 800, 975,1150}, 150, 130},
    /* 28 */ {"Water Works",       TILE_UTILITY,      GROUP_UTILITY,  150, {  0,  0,  0,   0,   0,   0},   0,  75},
    /* 29 */ {"Marvin Gardens",    TILE_PROPERTY,     GROUP_YELLOW,   280, { 24,120,360, 850,1025,1200}, 150, 140},
    /* 30 */ {"Go To Jail",        TILE_GO_TO_JAIL,   GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 31 */ {"Pacific Ave",       TILE_PROPERTY,     GROUP_GREEN,    300, { 26,130,390, 900,1100,1275}, 200, 150},
    /* 32 */ {"N. Carolina Ave",   TILE_PROPERTY,     GROUP_GREEN,    300, { 26,130,390, 900,1100,1275}, 200, 150},
    /* 33 */ {"Community Chest",   TILE_COMMUNITY,    GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 34 */ {"Pennsylvania Ave",  TILE_PROPERTY,     GROUP_GREEN,    320, { 28,150,450,1000,1200,1400}, 200, 160},
    /* 35 */ {"Short Line RR",     TILE_RAILROAD,     GROUP_RAILROAD, 200, { 25, 50,100, 200,   0,   0},   0, 100},
    /* 36 */ {"Chance",            TILE_CHANCE,       GROUP_NONE,       0, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 37 */ {"Park Place",        TILE_PROPERTY,     GROUP_DARK_BLUE,350, { 35,175,500,1100,1300,1500}, 200, 175},
    /* 38 */ {"Luxury Tax",        TILE_TAX,          GROUP_NONE,     100, {  0,  0,  0,   0,   0,   0},   0,   0},
    /* 39 */ {"Boardwalk",         TILE_PROPERTY,     GROUP_DARK_BLUE,400, { 50,200,600,1400,1700,2000}, 200, 200},
};

// =============================================================================
// CHANCE / COMMUNITY CHEST CARD DATA
// =============================================================================
enum CardEffect : uint8_t {
    CARD_MOVETO,          // Move to absolute position
    CARD_MOVEREL,         // Move relative spaces
    CARD_COLLECT,         // Collect $ from bank
    CARD_PAY,             // Pay $ to bank
    CARD_COLLECT_EACH,    // Collect $ from every other player
    CARD_PAY_EACH,        // Pay $ to every other player
    CARD_JAIL_FREE,       // Get-out-of-jail-free
    CARD_GO_JAIL,         // Go directly to jail
    CARD_REPAIRS,         // value1 = per house, value2 = per hotel
    CARD_NEAREST_RR,      // Advance to nearest railroad, pay double rent
    CARD_NEAREST_UTIL,    // Advance to nearest utility, pay 10× dice
};

struct CardData {
    const char* text;
    CardEffect  effect;
    int16_t     value1;
    int16_t     value2;
};

const CardData CHANCE_CARDS[16] = {
    {"Advance to GO.\nCollect $200.",                       CARD_MOVETO,       0,  0},
    {"Advance to Illinois Ave.",                            CARD_MOVETO,      24,  0},
    {"Advance to St. Charles Place.",                       CARD_MOVETO,      11,  0},
    {"Advance to nearest Utility.\nPay 10x dice if owned.",CARD_NEAREST_UTIL, 0,  0},
    {"Advance to nearest Railroad.\nPay double rent.",      CARD_NEAREST_RR,   0,  0},
    {"Bank pays you dividend of $50.",                      CARD_COLLECT,      50,  0},
    {"Get out of Jail free!",                               CARD_JAIL_FREE,    0,  0},
    {"Go back 3 spaces.",                                   CARD_MOVEREL,     -3,  0},
    {"Go directly to Jail.",                                CARD_GO_JAIL,      0,  0},
    {"Make general repairs.\n$25/house, $100/hotel.",       CARD_REPAIRS,      25,100},
    {"Pay poor tax of $15.",                                CARD_PAY,          15,  0},
    {"Take a ride on Reading RR.",                          CARD_MOVETO,       5,  0},
    {"Take a walk on Boardwalk.",                           CARD_MOVETO,      39,  0},
    {"You are elected chairman.\nPay each player $50.",     CARD_PAY_EACH,     50,  0},
    {"Building loan matures.\nCollect $150.",               CARD_COLLECT,     150,  0},
    {"You won a crossword\ncompetition! Collect $100.",     CARD_COLLECT,     100,  0},
};

const CardData COMMUNITY_CARDS[16] = {
    {"Advance to GO.\nCollect $200.",                       CARD_MOVETO,       0,  0},
    {"Bank error in your favour.\nCollect $200.",           CARD_COLLECT,     200,  0},
    {"Doctor's fee. Pay $50.",                              CARD_PAY,          50,  0},
    {"From sale of stock\nyou get $50.",                    CARD_COLLECT,      50,  0},
    {"Get out of Jail free!",                               CARD_JAIL_FREE,    0,  0},
    {"Go directly to Jail.",                                CARD_GO_JAIL,      0,  0},
    {"Grand Opera Night.\nCollect $50 from each player.",   CARD_COLLECT_EACH, 50,  0},
    {"Holiday fund matures.\nCollect $100.",                CARD_COLLECT,     100,  0},
    {"Income tax refund.\nCollect $20.",                    CARD_COLLECT,      20,  0},
    {"It's your birthday!\nCollect $10 from each player.",  CARD_COLLECT_EACH, 10,  0},
    {"Life insurance matures.\nCollect $100.",              CARD_COLLECT,     100,  0},
    {"Hospital fees. Pay $100.",                            CARD_PAY,         100,  0},
    {"School fees. Pay $50.",                               CARD_PAY,          50,  0},
    {"Receive consultancy fee.\nCollect $25.",              CARD_COLLECT,      25,  0},
    {"Street repairs.\n$40/house, $115/hotel.",             CARD_REPAIRS,      40,115},
    {"2nd prize beauty contest.\nCollect $10.",             CARD_COLLECT,      10,  0},
};

// =============================================================================
// HELPER: colour for property group
// =============================================================================
inline uint16_t groupColour(ColorGroup g) {
    switch (g) {
        case GROUP_BROWN:      return RGB565(139, 69, 19);
        case GROUP_LIGHT_BLUE: return RGB565(135,206,235);
        case GROUP_PINK:       return RGB565(216, 27, 96);
        case GROUP_ORANGE:     return RGB565(255,140,  0);
        case GROUP_RED:        return RGB565(255,  0,  0);
        case GROUP_YELLOW:     return RGB565(255,255,  0);
        case GROUP_GREEN:      return RGB565(  0,166, 81);
        case GROUP_DARK_BLUE:  return RGB565(  0,  0,200);
        case GROUP_RAILROAD:   return RGB565( 80, 80, 80);
        case GROUP_UTILITY:    return RGB565(200,200,200);
        default:               return 0x0000;
    }
}
