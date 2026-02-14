# ESP32 Monopoly Electronic Banking Firmware

Minimal card-driven firmware for ESP32 + ST7789 + PN532, matching the web emulator flow.

## Build

```bash
pio run
```

## Flash

```bash
pio run -t upload
```

## Implemented modules

- `src/nfc_manager.cpp`
- `src/card_manager.cpp`
- `src/game_logic.cpp`
- `src/display_ui.cpp`
- `src/battery_manager.cpp`
- `src/sound_manager.cpp`

## NFC block layout (Mifare Classic)

- Player bank card: block `4`
- Property card: block `8`
- Event card: block `12`

Each block stores:

- bytes `[0..2]`: magic `MB2`
- byte `[3]`: version
- byte `[4]`: card type
- remaining bytes: card payload

## Notes

- Game state follows card-driven state machine (`HOME`, `WAIT_CARD`, `PROPERTY_*`, `EVENT`, `AUCTION`, `DEBT`, `GO`, `JAIL`, `WINNER`).
- UI is intentionally sparse and monochrome-like, based on your reference photos.
- Property ownership/rent level are read/written from property cards.
