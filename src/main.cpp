#include <Arduino.h>
#include <FastLED.h>

// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
#include <ESP8266WiFi.h>
#include "secrets.hpp"
// 666 = 45 (15 x 3 )
// ARGHOUL = 97?
// ROAD = 59
#define LED_PIN D2
#define NUM_LEDS 201 // 666 + ARGHOUL + ROAD ==> 201 leds
#define BRIGHTNESS 32
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void setupWifi()
{
  Serial.begin(115200);
  Serial.println();
  // Serial.setDebugOutput(true);

  WiFi.begin(SSID_NAME, SSID_KEY);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void setup()
{

  delay(3000); // power-up safety delay

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  CRGB orange = CHSV(HUE_PURPLE, 255, 255);
  // CRGB orange = CHSV(HUE_RED, 255, 255);
  CRGB pumpkin = CHSV(24, 91, 100);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(orange, orange, orange, orange,
                                 pumpkin, pumpkin, pumpkin, pumpkin,
                                 orange, orange, orange, orange,
                                 pumpkin, pumpkin, pumpkin, pumpkin);

  currentBlending = LINEARBLEND;

  FillLEDsFromPaletteColors(0);

  setupWifi();
}

void loop()
{
  //    ChangePalettePeriodically();

  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}
