#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "config.h"

// =============================================================================
// NFC card data structures
// =============================================================================
struct NfcPlayerCard {
    uint8_t  type;                  // NFC_TYPE_PLAYER
    uint8_t  playerId;              // 0-7
    uint16_t colour;                // RGB565
    char     name[MAX_NAME_LEN+1];
};

struct NfcPropertyCard {
    uint8_t  type;                  // NFC_TYPE_PROPERTY
    uint8_t  tileIndex;             // 0-39
    uint8_t  group;                 // ColorGroup
    char     name[MAX_NAME_LEN+1];
};

struct NfcEventCard {
    uint8_t  type;                  // NFC_TYPE_EVENT
    uint8_t  eventId;               // Identifier
};

// =============================================================================
// PUBLIC API
// =============================================================================
bool    nfc_init();
bool    nfc_available();           // true if PN532 detected

// Blocking poll – returns true if a card appeared within timeoutMs
bool    nfc_pollCard(uint8_t* uid, uint8_t* uidLen, uint16_t timeoutMs = 500);

// Read card type (reads sector 1, block 4, byte 0)
uint8_t nfc_readCardType(uint8_t* uid, uint8_t uidLen);

// Typed reads
bool    nfc_readPlayerCard(uint8_t* uid, uint8_t uidLen, NfcPlayerCard& out);
bool    nfc_readPropertyCard(uint8_t* uid, uint8_t uidLen, NfcPropertyCard& out);
bool    nfc_readEventCard(uint8_t* uid, uint8_t uidLen, NfcEventCard& out);

// Typed writes (programming mode)
bool    nfc_writePlayerCard(uint8_t* uid, uint8_t uidLen, const NfcPlayerCard& data);
bool    nfc_writePropertyCard(uint8_t* uid, uint8_t uidLen, const NfcPropertyCard& data);
bool    nfc_writeEventCard(uint8_t* uid, uint8_t uidLen, const NfcEventCard& data);
