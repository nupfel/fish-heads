//#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0

// #define RACHEL

#include <Arduino.h>
#include <FastLED.h>

#ifdef RACHEL
#define LED_PIN     MOSI
#define CHIPSET     WS2812B
#define COLOUR_ORDER BGR
#else
#define CLOCK_PIN   SCK
#define DATA_PIN    MOSI
#define CHIPSET     SK9822
#define COLOUR_ORDER BGR
#endif

// #ifdef RACHEL
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
// #define NUM_LEDS    130
// #elif GERTRUDE
#define NUM_LEDS    130
// #endif

CRGB leds[NUM_LEDS];
CRGBPalette16 gPal = CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue);

void setup() {
  delay(1000); // sanity delay

  Serial.begin(115200);

#ifdef RACHEL
  FastLED.addLeds<CHIPSET, LED_PIN, COLOUR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#else
  FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLOUR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#endif

  FastLED.setBrightness(40);

  Serial.printf("\nOK %d\n", NUM_LEDS);
}

void loop() {
  static uint8_t i = 0;

  // fill_palette(leds, NUM_LEDS, i, 1, gPal, 255, LINEARBLEND);
  fill_rainbow(leds, NUM_LEDS, i, 255/NUM_LEDS);
  EVERY_N_MILLISECONDS(20) { i++; }

  FastLED.show();
  FastLED.delay(1000 / 100);
}
