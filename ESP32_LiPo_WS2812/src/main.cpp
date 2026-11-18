// Olimex ESP32-DevKit-LiPo - tira WS28xx en GPIO 33, 10 LEDs
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN     33
#define NUM_LEDS    10
#define BRIGHTNESS  50   // 0-255 (bajo para no sobrecargar con batería/USB)

// WS2812/WS2812B: NEO_GRB + NEO_KHZ800. Si los colores salen cambiados,
// prueba NEO_RGB. Para WS2811 a 400 kHz usa NEO_KHZ400.
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void colorWipe(uint32_t color, uint16_t wait) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint16_t wait, uint8_t loops) {
  for (uint32_t first = 0; first < 65536UL * loops; first += 256) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      uint16_t hue = first + (i * 65536UL / NUM_LEDS);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
    }
    strip.show();
    delay(wait);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("WS28xx en GPIO33 - 10 LEDs");
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  colorWipe(strip.Color(255, 0, 0), 80);   // rojo
  colorWipe(strip.Color(0, 255, 0), 80);   // verde
  colorWipe(strip.Color(0, 0, 255), 80);   // azul
  rainbow(10, 3);                          // arcoíris
}
