#include "nfc_handler.h"

// =============================================================================
// PN532 instance (I2C)
// =============================================================================
static Adafruit_PN532 _nfc(PIN_NFC_IRQ, PIN_NFC_RST);
static bool _nfcOk = false;

// Mifare Classic default key
static const uint8_t MIFARE_KEY[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// We use Sector 1 (blocks 4-7), block 4 = data row 1, block 5 = data row 2
#define DATA_BLOCK_1  4
#define DATA_BLOCK_2  5

// =============================================================================
// INIT
// =============================================================================
bool nfc_init() {
    Wire.begin(PIN_NFC_SDA, PIN_NFC_SCL);
    _nfc.begin();
    uint32_t ver = _nfc.getFirmwareVersion();
    if (!ver) {
        Serial.println(F("[NFC] PN532 not found"));
        _nfcOk = false;
        return false;
    }
    Serial.printf("[NFC] PN532 FW %d.%d\n", (ver >> 16) & 0xFF, (ver >> 8) & 0xFF);
    _nfc.SAMConfig();
    _nfcOk = true;
    return true;
}

bool nfc_available() { return _nfcOk; }

// =============================================================================
// POLL
// =============================================================================
bool nfc_pollCard(uint8_t* uid, uint8_t* uidLen, uint16_t timeoutMs) {
    if (!_nfcOk) return false;
    return _nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLen, timeoutMs);
}

// =============================================================================
// INTERNAL: authenticate sector 1
// =============================================================================
static bool _auth(uint8_t* uid, uint8_t uidLen, uint8_t block) {
    return _nfc.mifareclassic_AuthenticateBlock(uid, uidLen, block, 0,
                                                 const_cast<uint8_t*>(MIFARE_KEY));
}

static bool _readBlock(uint8_t* uid, uint8_t uidLen, uint8_t block, uint8_t* buf16) {
    if (!_auth(uid, uidLen, block)) return false;
    return _nfc.mifareclassic_ReadDataBlock(block, buf16);
}

static bool _writeBlock(uint8_t* uid, uint8_t uidLen, uint8_t block, uint8_t* buf16) {
    if (!_auth(uid, uidLen, block)) return false;
    return _nfc.mifareclassic_WriteDataBlock(block, buf16);
}

// =============================================================================
// READ CARD TYPE
// =============================================================================
uint8_t nfc_readCardType(uint8_t* uid, uint8_t uidLen) {
    uint8_t buf[16];
    if (!_readBlock(uid, uidLen, DATA_BLOCK_1, buf)) return 0;
    return buf[0];
}

// =============================================================================
// READ PLAYER CARD
//   Block 4: [type][playerId][colourH][colourL] ...
//   Block 5: name (up to 16 bytes)
// =============================================================================
bool nfc_readPlayerCard(uint8_t* uid, uint8_t uidLen, NfcPlayerCard& out) {
    uint8_t b1[16], b2[16];
    if (!_readBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    if (b1[0] != NFC_TYPE_PLAYER) return false;
    // Must re-auth for next block in same sector after read
    if (!_readBlock(uid, uidLen, DATA_BLOCK_2, b2)) return false;

    out.type     = b1[0];
    out.playerId = b1[1];
    out.colour   = (b1[2] << 8) | b1[3];
    memset(out.name, 0, sizeof(out.name));
    memcpy(out.name, b2, min((int)sizeof(out.name)-1, 16));
    return true;
}

// =============================================================================
// READ PROPERTY CARD
//   Block 4: [type][tileIndex][group] ...
//   Block 5: name
// =============================================================================
bool nfc_readPropertyCard(uint8_t* uid, uint8_t uidLen, NfcPropertyCard& out) {
    uint8_t b1[16], b2[16];
    if (!_readBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    if (b1[0] != NFC_TYPE_PROPERTY) return false;
    if (!_readBlock(uid, uidLen, DATA_BLOCK_2, b2)) return false;

    out.type      = b1[0];
    out.tileIndex = b1[1];
    out.group     = b1[2];
    memset(out.name, 0, sizeof(out.name));
    memcpy(out.name, b2, min((int)sizeof(out.name)-1, 16));
    return true;
}

// =============================================================================
// READ EVENT CARD
//   Block 4: [type][eventId] ...
// =============================================================================
bool nfc_readEventCard(uint8_t* uid, uint8_t uidLen, NfcEventCard& out) {
    uint8_t b1[16];
    if (!_readBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    if (b1[0] != NFC_TYPE_EVENT) return false;
    out.type    = b1[0];
    out.eventId = b1[1];
    return true;
}

// =============================================================================
// WRITE PLAYER CARD
// =============================================================================
bool nfc_writePlayerCard(uint8_t* uid, uint8_t uidLen, const NfcPlayerCard& data) {
    uint8_t b1[16] = {0};
    uint8_t b2[16] = {0};
    b1[0] = NFC_TYPE_PLAYER;
    b1[1] = data.playerId;
    b1[2] = (data.colour >> 8) & 0xFF;
    b1[3] = data.colour & 0xFF;
    strncpy((char*)b2, data.name, 15);

    if (!_writeBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    if (!_writeBlock(uid, uidLen, DATA_BLOCK_2, b2)) return false;
    return true;
}

// =============================================================================
// WRITE PROPERTY CARD
// =============================================================================
bool nfc_writePropertyCard(uint8_t* uid, uint8_t uidLen, const NfcPropertyCard& data) {
    uint8_t b1[16] = {0};
    uint8_t b2[16] = {0};
    b1[0] = NFC_TYPE_PROPERTY;
    b1[1] = data.tileIndex;
    b1[2] = data.group;
    strncpy((char*)b2, data.name, 15);

    if (!_writeBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    if (!_writeBlock(uid, uidLen, DATA_BLOCK_2, b2)) return false;
    return true;
}

// =============================================================================
// WRITE EVENT CARD
// =============================================================================
bool nfc_writeEventCard(uint8_t* uid, uint8_t uidLen, const NfcEventCard& data) {
    uint8_t b1[16] = {0};
    b1[0] = NFC_TYPE_EVENT;
    b1[1] = data.eventId;

    if (!_writeBlock(uid, uidLen, DATA_BLOCK_1, b1)) return false;
    return true;
}
