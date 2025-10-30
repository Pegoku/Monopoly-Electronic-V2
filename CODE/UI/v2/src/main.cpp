#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS 14
#define TFT_DC 16
#define TFT_RST 38
#define TFT_SCLK 12
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_LED 21

#define TFT_WIDTH 240
#define TFT_HEIGHT 320

constexpr uint32_t kColorHoldMs = 800;

constexpr uint16_t kColorCycle[] = {
    ST77XX_BLACK,
    ST77XX_RED,
    ST77XX_GREEN,
    ST77XX_BLUE,
    ST77XX_YELLOW,
    ST77XX_MAGENTA,
    ST77XX_CYAN,
    ST77XX_WHITE};

constexpr size_t kColorCount = sizeof(kColorCycle) / sizeof(kColorCycle[0]);

Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);

void drawCenteredText(const char *msg, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextSize(2);
  tft.setTextWrap(false);

  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);

  const int16_t x = (TFT_WIDTH - static_cast<int16_t>(w)) / 2 - x1;
  const int16_t y = (TFT_HEIGHT - static_cast<int16_t>(h)) / 2 - y1;
  tft.setCursor(x, y);
  tft.print(msg);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);  // Turn on backlight

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  drawCenteredText("Hello ST7789", ST77XX_WHITE);
}

void loop() {
  static uint32_t lastSwitchMs = 0;
  static size_t colorIndex = 0;

  const uint32_t now = millis();
  if (now - lastSwitchMs < kColorHoldMs) {
    return;
  }

  lastSwitchMs = now;
  const uint16_t bg = kColorCycle[colorIndex];
  const uint16_t textColor = (bg == ST77XX_BLACK) ? ST77XX_WHITE : ST77XX_BLACK;

  tft.fillScreen(bg);
  drawCenteredText("Hello ST7789", textColor);

  colorIndex = (colorIndex + 1U) % kColorCount;
}