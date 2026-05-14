/// @file    ColorTemperature.ino
/// @brief   Demonstrates how to use @ref ColorTemperature based color correction
/// @example ColorTemperature.ino

#include <FastLED.h>

#define LED_PIN     4

// Information about the LED strip itself
#define NUM_LEDS    45
#define CHIPSET     WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define BRIGHTNESS  255
#define TEMPERATURE_1 Tungsten100W
#define TEMPERATURE_2 OvercastSky

void loop()
{
  // draw a generic, no-name rainbow
  static uint8_t starthue = 0;
  fill_rainbow( leds, NUM_LEDS, --starthue, 20);

  FastLED.setTemperature( TEMPERATURE_2 );
  FastLED.show();
  FastLED.delay(20);
}

void setup() {
  delay( 300 ); // power-up safety delay
  // It's important to set the color correction for your LED strip here,
  // so that colors can be more accurately rendered through the 'temperature' profiles
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness( BRIGHTNESS );
}

