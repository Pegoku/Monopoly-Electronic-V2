#pragma once
#include <Arduino.h>
#include "config.h"
#include "game_data.h"

// =============================================================================
// PLAYER
// =============================================================================
struct Player {
    char     name[MAX_NAME_LEN + 1] = "";
    int32_t  money        = 0;
    uint8_t  position     = 0;
    uint16_t colour       = 0;
    bool     alive        = false;          // still in game
    bool     inJail       = false;
    uint8_t  jailTurns    = 0;
    bool     hasJailCard  = false;          // get-out-of-jail-free
    uint8_t  uid[7]       = {};             // NFC UID
    uint8_t  uidLen       = 0;
    uint8_t  doublesCount = 0;

    // Bit-mask of owned tile indices (bits 0-39)
    uint64_t ownedTiles   = 0;
};

// =============================================================================
// PROPERTY STATE (per tile)
// =============================================================================
struct PropertyState {
    int8_t  owner      = -1;               // -1 = unowned / bank
    uint8_t houses     = 0;                // 0-4 houses, 5 = hotel
    bool    mortgaged  = false;
};

// =============================================================================
// GAME PHASE STATE MACHINE
// =============================================================================
enum GamePhase : uint8_t {
    PHASE_SPLASH = 0,
    PHASE_MENU,
    PHASE_SETUP_COUNT,
    PHASE_SETUP_PLAYERS,
    PHASE_TURN_START,
    PHASE_ROLLING,
    PHASE_MOVED,
    PHASE_TILE_ACTION,
    PHASE_CARD_DRAW,
    PHASE_JAIL_TURN,
    PHASE_TRADE_SELECT,
    PHASE_TRADE_OFFER,
    PHASE_QUICK_MENU,
    PHASE_PROGRAMMING,
    PHASE_SETTINGS,
    PHASE_GAME_OVER,
};

// Sub-actions for PHASE_TILE_ACTION
enum TileAction : uint8_t {
    ACT_NONE = 0,
    ACT_BUY,              // unowned property
    ACT_PAY_RENT,         // owned by other
    ACT_OWN_PROP,         // own property detail
    ACT_TAX,
    ACT_FREE_PARKING,
    ACT_GO_TO_JAIL,
    ACT_JUST_VISITING,
};

// =============================================================================
// GAME STATE
// =============================================================================
struct GameState {
    GameSettings settings;

    // Players
    Player  players[MAX_PLAYERS];
    uint8_t numPlayers      = 0;
    uint8_t currentPlayer   = 0;
    uint8_t alivePlayers    = 0;

    // Board
    PropertyState props[BOARD_SIZE];

    // Dice
    uint8_t dice1 = 0, dice2 = 0;
    bool    isDoubles       = false;

    // Phase
    GamePhase phase         = PHASE_SPLASH;
    TileAction tileAction   = ACT_NONE;

    // Card draw
    bool        cardIsChance = false;
    uint8_t     cardIndex    = 0;

    // Pools / tax
    int32_t     freeParkingPool = 0;

    // Trade state
    uint8_t     tradeWith    = 0;
    int32_t     tradeMoneyOffer   = 0;
    int32_t     tradeMoneyRequest = 0;
    uint64_t    tradePropsOffer   = 0;
    uint64_t    tradePropsRequest = 0;

    // Card deck shuffling
    uint8_t     chanceDeck[NUM_CHANCE_CARDS];
    uint8_t     communityDeck[NUM_COMMUNITY_CARDS];
    uint8_t     chanceIdx     = 0;
    uint8_t     communityIdx  = 0;

    // Turn history
    uint16_t    turnNumber    = 0;

    // Dirty flags for UI
    bool        screenDirty   = true;
};

extern GameState G;

// =============================================================================
// GAME ENGINE API
// =============================================================================

// Initialisation
void game_init();                            // Reset everything
void game_newGame(uint8_t numPlayers);       // Start new game
void game_shuffleDecks();

// Turn flow
void game_startTurn();
void game_rollDice();                        // Sets dice1, dice2, isDoubles
void game_movePlayer();                      // Moves current player by dice total
void game_resolveTile();                     // Determines tile action

// Actions
bool game_buyProperty(uint8_t playerIdx, uint8_t tileIdx);
int32_t game_calcRent(uint8_t tileIdx, uint8_t diceTotal);
bool game_payRent(uint8_t fromPlayer, uint8_t tileIdx);
bool game_buildHouse(uint8_t playerIdx, uint8_t tileIdx);
bool game_sellHouse(uint8_t playerIdx, uint8_t tileIdx);
bool game_mortgageProperty(uint8_t playerIdx, uint8_t tileIdx);
bool game_unmortgageProperty(uint8_t playerIdx, uint8_t tileIdx);
void game_payBank(uint8_t playerIdx, int32_t amount);
void game_collectFromBank(uint8_t playerIdx, int32_t amount);

// Jail
void game_sendToJail(uint8_t playerIdx);
bool game_tryJailRoll(uint8_t playerIdx);
void game_payJailFine(uint8_t playerIdx);
void game_useJailCard(uint8_t playerIdx);

// Cards
void game_drawCard(bool isChance);
void game_applyCard(const CardData& card);

// Trade
bool game_executeTrade();

// Turn management
void game_endTurn();
void game_checkBankruptcy(uint8_t playerIdx);
bool game_isGameOver();
uint8_t game_getWinner();

// Queries
bool game_ownsFullGroup(uint8_t playerIdx, ColorGroup group);
uint8_t game_countInGroup(uint8_t playerIdx, ColorGroup group);
uint8_t game_playerRailroads(uint8_t playerIdx);
uint8_t game_playerUtilities(uint8_t playerIdx);
