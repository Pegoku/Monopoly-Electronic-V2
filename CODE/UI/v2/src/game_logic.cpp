#include "game_logic.h"

GameState G;

// =============================================================================
// HELPERS
// =============================================================================
static void _shuffleArray(uint8_t* arr, uint8_t len) {
    for (uint8_t i = len - 1; i > 0; i--) {
        uint8_t j = random(0, i + 1);
        uint8_t t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

// =============================================================================
// INIT / NEW GAME
// =============================================================================
void game_init() {
    memset(&G, 0, sizeof(G));
    G.phase = PHASE_SPLASH;
    G.screenDirty = true;
    for (auto& p : G.props) { p.owner = -1; p.houses = 0; p.mortgaged = false; }
}

void game_shuffleDecks() {
    for (uint8_t i = 0; i < NUM_CHANCE_CARDS; i++)    G.chanceDeck[i] = i;
    for (uint8_t i = 0; i < NUM_COMMUNITY_CARDS; i++) G.communityDeck[i] = i;
    _shuffleArray(G.chanceDeck, NUM_CHANCE_CARDS);
    _shuffleArray(G.communityDeck, NUM_COMMUNITY_CARDS);
    G.chanceIdx = 0;
    G.communityIdx = 0;
}

void game_newGame(uint8_t numPlayers) {
    game_init();
    G.numPlayers = numPlayers;
    G.alivePlayers = numPlayers;
    for (uint8_t i = 0; i < numPlayers; i++) {
        Player& p = G.players[i];
        p.alive    = true;
        p.money    = G.settings.startingMoney;
        p.position = 0;
        p.colour   = PLAYER_COLOURS[i];
        snprintf(p.name, MAX_NAME_LEN + 1, "Player %d", i + 1);
    }
    game_shuffleDecks();
    G.phase = PHASE_TURN_START;
    G.currentPlayer = 0;
    G.turnNumber = 1;
    G.screenDirty = true;
}

// =============================================================================
// TURN FLOW
// =============================================================================
void game_startTurn() {
    Player& p = G.players[G.currentPlayer];
    p.doublesCount = 0;
    if (p.inJail) {
        G.phase = PHASE_JAIL_TURN;
    } else {
        G.phase = PHASE_TURN_START;
    }
    G.screenDirty = true;
}

void game_rollDice() {
    G.dice1 = random(1, 7);
    G.dice2 = random(1, 7);
    G.isDoubles = (G.dice1 == G.dice2);
    G.players[G.currentPlayer].doublesCount += G.isDoubles ? 1 : 0;
}

void game_movePlayer() {
    Player& p = G.players[G.currentPlayer];
    // Three doubles → jail
    if (p.doublesCount >= 3) {
        game_sendToJail(G.currentPlayer);
        G.phase = PHASE_TILE_ACTION;
        G.tileAction = ACT_GO_TO_JAIL;
        G.screenDirty = true;
        return;
    }
    uint8_t total = G.dice1 + G.dice2;
    uint8_t oldPos = p.position;
    p.position = (p.position + total) % BOARD_SIZE;
    // Passed GO?
    if (p.position < oldPos && p.position != 0) {
        p.money += GO_SALARY;
    }
    // Landed exactly on GO
    if (p.position == 0 && oldPos != 0) {
        p.money += GO_SALARY;
    }
    G.phase = PHASE_MOVED;
    G.screenDirty = true;
}

void game_resolveTile() {
    Player& p = G.players[G.currentPlayer];
    const TileData& tile = TILES[p.position];

    switch (tile.type) {
        case TILE_GO:
            G.tileAction = ACT_NONE; // Already collected when passing
            break;

        case TILE_PROPERTY:
        case TILE_RAILROAD:
        case TILE_UTILITY: {
            int8_t owner = G.props[p.position].owner;
            if (owner == -1) {
                G.tileAction = ACT_BUY;
            } else if (owner == (int8_t)G.currentPlayer) {
                G.tileAction = ACT_OWN_PROP;
            } else if (G.props[p.position].mortgaged) {
                G.tileAction = ACT_NONE; // Mortgaged = no rent
            } else {
                G.tileAction = ACT_PAY_RENT;
            }
            break;
        }
        case TILE_CHANCE:
            G.tileAction = ACT_NONE;
            game_drawCard(true);
            G.phase = PHASE_CARD_DRAW;
            G.screenDirty = true;
            return;

        case TILE_COMMUNITY:
            G.tileAction = ACT_NONE;
            game_drawCard(false);
            G.phase = PHASE_CARD_DRAW;
            G.screenDirty = true;
            return;

        case TILE_TAX:
            G.tileAction = ACT_TAX;
            break;

        case TILE_JAIL:
            G.tileAction = ACT_JUST_VISITING;
            break;

        case TILE_FREE_PARKING:
            G.tileAction = ACT_FREE_PARKING;
            break;

        case TILE_GO_TO_JAIL:
            G.tileAction = ACT_GO_TO_JAIL;
            game_sendToJail(G.currentPlayer);
            break;
    }
    G.phase = PHASE_TILE_ACTION;
    G.screenDirty = true;
}

// =============================================================================
// PROPERTY ACTIONS
// =============================================================================
bool game_buyProperty(uint8_t playerIdx, uint8_t tileIdx) {
    const TileData& tile = TILES[tileIdx];
    Player& p = G.players[playerIdx];
    if (G.props[tileIdx].owner != -1) return false;
    if (p.money < tile.price) return false;

    p.money -= tile.price;
    G.props[tileIdx].owner = playerIdx;
    p.ownedTiles |= (1ULL << tileIdx);
    return true;
}

int32_t game_calcRent(uint8_t tileIdx, uint8_t diceTotal) {
    const TileData& tile = TILES[tileIdx];
    const PropertyState& ps = G.props[tileIdx];
    if (ps.owner < 0 || ps.mortgaged) return 0;

    uint8_t owner = ps.owner;

    if (tile.type == TILE_RAILROAD) {
        uint8_t count = game_playerRailroads(owner);
        if (count == 0) return 0;
        return tile.rent[count - 1]; // 25, 50, 100, 200
    }

    if (tile.type == TILE_UTILITY) {
        uint8_t count = game_playerUtilities(owner);
        return (count >= 2) ? diceTotal * 10 : diceTotal * 4;
    }

    // Normal property
    if (ps.houses > 0) {
        return tile.rent[ps.houses]; // 1-5 (hotel = index 5)
    }
    // Base rent, doubled if monopoly
    int32_t base = tile.rent[0];
    if (game_ownsFullGroup(owner, tile.group)) base *= 2;
    return base;
}

bool game_payRent(uint8_t fromPlayer, uint8_t tileIdx) {
    uint8_t diceTotal = G.dice1 + G.dice2;
    int32_t rent = game_calcRent(tileIdx, diceTotal);
    int8_t owner = G.props[tileIdx].owner;
    if (owner < 0 || rent <= 0) return false;

    G.players[fromPlayer].money -= rent;
    G.players[owner].money += rent;
    game_checkBankruptcy(fromPlayer);
    return true;
}

bool game_buildHouse(uint8_t playerIdx, uint8_t tileIdx) {
    const TileData& tile = TILES[tileIdx];
    Player& p = G.players[playerIdx];
    PropertyState& ps = G.props[tileIdx];

    if (tile.type != TILE_PROPERTY) return false;
    if (ps.owner != (int8_t)playerIdx) return false;
    if (!game_ownsFullGroup(playerIdx, tile.group)) return false;
    if (ps.houses >= MAX_HOUSES) return false;
    if (ps.mortgaged) return false;
    if (p.money < tile.houseCost) return false;

    // Even building rule: can't build if any same-group property has fewer houses
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (TILES[i].group == tile.group && i != tileIdx) {
            if (G.props[i].houses < ps.houses) return false;
        }
    }

    p.money -= tile.houseCost;
    ps.houses++;
    return true;
}

bool game_sellHouse(uint8_t playerIdx, uint8_t tileIdx) {
    const TileData& tile = TILES[tileIdx];
    PropertyState& ps = G.props[tileIdx];
    if (ps.owner != (int8_t)playerIdx) return false;
    if (ps.houses == 0) return false;

    // Even selling: can't sell if any same-group has more houses
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (TILES[i].group == tile.group && i != tileIdx) {
            if (G.props[i].houses > ps.houses) return false;
        }
    }

    ps.houses--;
    G.players[playerIdx].money += tile.houseCost / 2;
    return true;
}

bool game_mortgageProperty(uint8_t playerIdx, uint8_t tileIdx) {
    PropertyState& ps = G.props[tileIdx];
    if (ps.owner != (int8_t)playerIdx) return false;
    if (ps.mortgaged) return false;
    if (ps.houses > 0) return false;  // Must sell houses first

    ps.mortgaged = true;
    G.players[playerIdx].money += TILES[tileIdx].mortgage;
    return true;
}

bool game_unmortgageProperty(uint8_t playerIdx, uint8_t tileIdx) {
    PropertyState& ps = G.props[tileIdx];
    if (ps.owner != (int8_t)playerIdx) return false;
    if (!ps.mortgaged) return false;

    int32_t cost = TILES[tileIdx].mortgage + (TILES[tileIdx].mortgage / 10); // 110%
    if (G.players[playerIdx].money < cost) return false;

    ps.mortgaged = false;
    G.players[playerIdx].money -= cost;
    return true;
}

void game_payBank(uint8_t playerIdx, int32_t amount) {
    G.players[playerIdx].money -= amount;
    if (G.settings.freeParkingPool) G.freeParkingPool += amount;
    game_checkBankruptcy(playerIdx);
}

void game_collectFromBank(uint8_t playerIdx, int32_t amount) {
    G.players[playerIdx].money += amount;
}

// =============================================================================
// JAIL
// =============================================================================
void game_sendToJail(uint8_t playerIdx) {
    Player& p = G.players[playerIdx];
    p.position = JAIL_POSITION;
    p.inJail   = true;
    p.jailTurns = 0;
    p.doublesCount = 0;
}

bool game_tryJailRoll(uint8_t playerIdx) {
    game_rollDice();
    Player& p = G.players[playerIdx];
    p.jailTurns++;
    if (G.isDoubles) {
        p.inJail = false;
        p.jailTurns = 0;
        return true;  // Player is free, move normally
    }
    if (p.jailTurns >= G.settings.jailMaxTurns) {
        // Forced to pay
        game_payJailFine(playerIdx);
        return true;
    }
    return false;  // Still in jail
}

void game_payJailFine(uint8_t playerIdx) {
    Player& p = G.players[playerIdx];
    p.money -= JAIL_FINE;
    p.inJail = false;
    p.jailTurns = 0;
    if (G.settings.freeParkingPool) G.freeParkingPool += JAIL_FINE;
    game_checkBankruptcy(playerIdx);
}

void game_useJailCard(uint8_t playerIdx) {
    Player& p = G.players[playerIdx];
    if (p.hasJailCard) {
        p.hasJailCard = false;
        p.inJail = false;
        p.jailTurns = 0;
    }
}

// =============================================================================
// CARDS
// =============================================================================
void game_drawCard(bool isChance) {
    G.cardIsChance = isChance;
    if (isChance) {
        G.cardIndex = G.chanceDeck[G.chanceIdx];
        G.chanceIdx = (G.chanceIdx + 1) % NUM_CHANCE_CARDS;
    } else {
        G.cardIndex = G.communityDeck[G.communityIdx];
        G.communityIdx = (G.communityIdx + 1) % NUM_COMMUNITY_CARDS;
    }
}

void game_applyCard(const CardData& card) {
    Player& p = G.players[G.currentPlayer];
    uint8_t cp = G.currentPlayer;

    switch (card.effect) {
        case CARD_MOVETO: {
            uint8_t dest = (uint8_t)card.value1;
            if (dest < p.position && dest != JAIL_POSITION) {
                p.money += GO_SALARY;  // passed GO
            }
            p.position = dest;
            break;
        }
        case CARD_MOVEREL: {
            int8_t delta = (int8_t)card.value1;
            p.position = (uint8_t)((int16_t)p.position + delta + BOARD_SIZE) % BOARD_SIZE;
            break;
        }
        case CARD_COLLECT:
            p.money += card.value1;
            break;

        case CARD_PAY:
            game_payBank(cp, card.value1);
            break;

        case CARD_COLLECT_EACH:
            for (uint8_t i = 0; i < G.numPlayers; i++) {
                if (i != cp && G.players[i].alive) {
                    G.players[i].money -= card.value1;
                    p.money += card.value1;
                    game_checkBankruptcy(i);
                }
            }
            break;

        case CARD_PAY_EACH:
            for (uint8_t i = 0; i < G.numPlayers; i++) {
                if (i != cp && G.players[i].alive) {
                    p.money -= card.value1;
                    G.players[i].money += card.value1;
                }
            }
            game_checkBankruptcy(cp);
            break;

        case CARD_JAIL_FREE:
            p.hasJailCard = true;
            break;

        case CARD_GO_JAIL:
            game_sendToJail(cp);
            break;

        case CARD_REPAIRS: {
            int32_t cost = 0;
            for (int i = 0; i < BOARD_SIZE; i++) {
                if (G.props[i].owner == (int8_t)cp) {
                    if (G.props[i].houses == 5) cost += card.value2;       // hotel
                    else                        cost += G.props[i].houses * card.value1;
                }
            }
            game_payBank(cp, cost);
            break;
        }
        case CARD_NEAREST_RR: {
            // Find nearest railroad forward
            uint8_t pos = p.position;
            uint8_t rrs[] = {5, 15, 25, 35};
            uint8_t nearest = rrs[0];
            for (int i = 0; i < 4; i++) {
                if (rrs[i] > pos) { nearest = rrs[i]; break; }
                if (i == 3) { nearest = rrs[0]; } // wrap around
            }
            if (nearest <= p.position) p.money += GO_SALARY;
            p.position = nearest;
            // Pay double rent if owned
            if (G.props[nearest].owner >= 0 && G.props[nearest].owner != (int8_t)cp) {
                int32_t rent = game_calcRent(nearest, G.dice1 + G.dice2) * 2;
                G.players[cp].money -= rent;
                G.players[G.props[nearest].owner].money += rent;
                game_checkBankruptcy(cp);
            }
            break;
        }
        case CARD_NEAREST_UTIL: {
            uint8_t pos = p.position;
            uint8_t nearest = (pos < 12 || pos >= 28) ? 12 : 28;
            if (nearest <= p.position && nearest != 12) p.money += GO_SALARY;
            p.position = nearest;
            // Pay 10× dice if owned
            if (G.props[nearest].owner >= 0 && G.props[nearest].owner != (int8_t)cp) {
                int32_t rent = (G.dice1 + G.dice2) * 10;
                G.players[cp].money -= rent;
                G.players[G.props[nearest].owner].money += rent;
                game_checkBankruptcy(cp);
            }
            break;
        }
    }
}

// =============================================================================
// TRADE
// =============================================================================
bool game_executeTrade() {
    uint8_t cp = G.currentPlayer;
    uint8_t tp = G.tradeWith;
    Player& me    = G.players[cp];
    Player& them  = G.players[tp];

    // Validate money
    if (me.money < G.tradeMoneyOffer) return false;
    if (them.money < G.tradeMoneyRequest) return false;

    // Validate property ownership
    if ((me.ownedTiles & G.tradePropsOffer) != G.tradePropsOffer) return false;
    if ((them.ownedTiles & G.tradePropsRequest) != G.tradePropsRequest) return false;

    // Execute money
    me.money   -= G.tradeMoneyOffer;
    them.money += G.tradeMoneyOffer;
    me.money   += G.tradeMoneyRequest;
    them.money -= G.tradeMoneyRequest;

    // Transfer offered properties
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (G.tradePropsOffer & (1ULL << i)) {
            G.props[i].owner = tp;
            me.ownedTiles   &= ~(1ULL << i);
            them.ownedTiles |= (1ULL << i);
        }
        if (G.tradePropsRequest & (1ULL << i)) {
            G.props[i].owner = cp;
            them.ownedTiles &= ~(1ULL << i);
            me.ownedTiles   |= (1ULL << i);
        }
    }

    // Reset trade state
    G.tradeMoneyOffer = G.tradeMoneyRequest = 0;
    G.tradePropsOffer = G.tradePropsRequest = 0;
    return true;
}

// =============================================================================
// TURN MANAGEMENT
// =============================================================================
void game_endTurn() {
    Player& p = G.players[G.currentPlayer];
    if (G.isDoubles && p.alive && !p.inJail) {
        // Roll again on doubles
        G.phase = PHASE_TURN_START;
        G.screenDirty = true;
        return;
    }
    // Next player
    do {
        G.currentPlayer = (G.currentPlayer + 1) % G.numPlayers;
    } while (!G.players[G.currentPlayer].alive);
    G.turnNumber++;
    game_startTurn();
}

void game_checkBankruptcy(uint8_t playerIdx) {
    Player& p = G.players[playerIdx];
    if (p.money < 0) {
        // For simplicity: auto-bankrupt (real game would offer mortgage/sell)
        // TODO: offer player chance to mortgage / sell before going bankrupt
        p.alive = false;
        G.alivePlayers--;
        // Return properties to bank
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (G.props[i].owner == (int8_t)playerIdx) {
                G.props[i].owner = -1;
                G.props[i].houses = 0;
                G.props[i].mortgaged = false;
            }
        }
        p.ownedTiles = 0;
        if (game_isGameOver()) {
            G.phase = PHASE_GAME_OVER;
            G.screenDirty = true;
        }
    }
}

bool game_isGameOver() {
    return G.alivePlayers <= 1;
}

uint8_t game_getWinner() {
    for (uint8_t i = 0; i < G.numPlayers; i++) {
        if (G.players[i].alive) return i;
    }
    return 0;
}

// =============================================================================
// QUERIES
// =============================================================================
bool game_ownsFullGroup(uint8_t playerIdx, ColorGroup group) {
    if (group == GROUP_NONE) return false;
    uint8_t needed = GROUP_SIZE[group];
    return game_countInGroup(playerIdx, group) >= needed;
}

uint8_t game_countInGroup(uint8_t playerIdx, ColorGroup group) {
    uint8_t count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (TILES[i].group == group && G.props[i].owner == (int8_t)playerIdx)
            count++;
    }
    return count;
}

uint8_t game_playerRailroads(uint8_t playerIdx) {
    return game_countInGroup(playerIdx, GROUP_RAILROAD);
}

uint8_t game_playerUtilities(uint8_t playerIdx) {
    return game_countInGroup(playerIdx, GROUP_UTILITY);
}
