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

#define TFT_T_CS 17
#define TFT_T_IRQ 18

#define TFT_WIDTH 240
#define TFT_HEIGHT 320

Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);


void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);  // Turn on backlight

  pinMode(TFT_T_CS, OUTPUT);
  digitalWrite(TFT_T_CS, HIGH);  // Keep touch controller deselected when idle

  pinMode(TFT_T_IRQ, INPUT_PULLUP);  // XPT2046-style controllers pull this low on touch

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  static bool lastTouch = false;

  tft.fillScreen(ST77XX_YELLOW);

  const bool touchActive = (digitalRead(TFT_T_IRQ) == LOW);
  if (touchActive != lastTouch) {
    if (touchActive) {
      Serial.println("Touch detected");
    } else {
      Serial.println("Touch released");
    }
    lastTouch = touchActive;
  }

  delay(20);
}