#include "storage.h"
#include <Preferences.h>

static Preferences prefs;

// =============================================================================
// GAME SAVE / LOAD  (binary blob into NVS)
// =============================================================================

// We serialise the parts of GameState that matter into a flat buffer.
// Max NVS blob ~500 KB on ESP32-S3 default partition, our data < 2 KB.

struct SaveHeader {
    uint32_t magic;       // 0x4D4F4E4F  "MONO"
    uint8_t  version;
    uint8_t  numPlayers;
    uint8_t  currentPlayer;
    uint8_t  alivePlayers;
    uint16_t turnNumber;
    int32_t  freeParkingPool;
};

bool storage_saveGame() {
    prefs.begin("monopoly", false);

    // Header
    SaveHeader hdr;
    hdr.magic           = 0x4D4F4E4F;
    hdr.version         = 1;
    hdr.numPlayers      = G.numPlayers;
    hdr.currentPlayer   = G.currentPlayer;
    hdr.alivePlayers    = G.alivePlayers;
    hdr.turnNumber      = G.turnNumber;
    hdr.freeParkingPool = G.freeParkingPool;
    prefs.putBytes("hdr", &hdr, sizeof(hdr));

    // Players (one blob)
    prefs.putBytes("players", G.players, sizeof(Player) * MAX_PLAYERS);

    // Properties
    prefs.putBytes("props", G.props, sizeof(PropertyState) * BOARD_SIZE);

    // Card deck state
    prefs.putBytes("cdeck", G.chanceDeck, NUM_CHANCE_CARDS);
    prefs.putBytes("cdecki", &G.chanceIdx, 1);
    prefs.putBytes("comdeck", G.communityDeck, NUM_COMMUNITY_CARDS);
    prefs.putBytes("comdecki", &G.communityIdx, 1);

    // Settings
    prefs.putBytes("settings", &G.settings, sizeof(GameSettings));

    prefs.end();
    Serial.println(F("[STORAGE] Game saved"));
    return true;
}

bool storage_loadGame() {
    prefs.begin("monopoly", true);

    SaveHeader hdr;
    if (prefs.getBytes("hdr", &hdr, sizeof(hdr)) != sizeof(hdr)) {
        prefs.end();
        return false;
    }
    if (hdr.magic != 0x4D4F4E4F || hdr.version != 1) {
        prefs.end();
        return false;
    }

    G.numPlayers      = hdr.numPlayers;
    G.currentPlayer   = hdr.currentPlayer;
    G.alivePlayers    = hdr.alivePlayers;
    G.turnNumber      = hdr.turnNumber;
    G.freeParkingPool = hdr.freeParkingPool;

    prefs.getBytes("players",  G.players,       sizeof(Player) * MAX_PLAYERS);
    prefs.getBytes("props",    G.props,          sizeof(PropertyState) * BOARD_SIZE);
    prefs.getBytes("cdeck",    G.chanceDeck,     NUM_CHANCE_CARDS);
    prefs.getBytes("cdecki",   &G.chanceIdx,     1);
    prefs.getBytes("comdeck",  G.communityDeck,  NUM_COMMUNITY_CARDS);
    prefs.getBytes("comdecki", &G.communityIdx,  1);
    prefs.getBytes("settings", &G.settings,      sizeof(GameSettings));

    prefs.end();
    G.phase = PHASE_TURN_START;
    G.screenDirty = true;
    Serial.println(F("[STORAGE] Game loaded"));
    return true;
}

bool storage_hasSavedGame() {
    prefs.begin("monopoly", true);
    SaveHeader hdr;
    bool ok = (prefs.getBytes("hdr", &hdr, sizeof(hdr)) == sizeof(hdr))
              && hdr.magic == 0x4D4F4E4F;
    prefs.end();
    return ok;
}

void storage_clearSave() {
    prefs.begin("monopoly", false);
    prefs.clear();
    prefs.end();
}

// =============================================================================
// SETTINGS PERSISTENCE
// =============================================================================
bool storage_saveSettings(const GameSettings& s) {
    prefs.begin("monosett", false);
    prefs.putBytes("s", &s, sizeof(GameSettings));
    prefs.end();
    return true;
}

bool storage_loadSettings(GameSettings& s) {
    prefs.begin("monosett", true);
    bool ok = prefs.getBytes("s", &s, sizeof(GameSettings)) == sizeof(GameSettings);
    prefs.end();
    return ok;
}
