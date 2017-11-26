#include <ESP8266WiFi.h> // Include the ESP8266 Library
#include <Adafruit_NeoPixel.h> // Include the adafruit Neopixel Library
 
#define PIN     2
#define N_LEDS 52
 
Adafruit_NeoPixel rings = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRBW + NEO_KHZ800);
 
void setup() {
  rings.setBrightness(30);
  rings.begin();
}
 
void loop() {
  chase(rings.Color(255, 0, 0)); // Red
  chase(rings.Color(0, 255, 0)); // Green
  chase(rings.Color(0, 0, 255)); // Blue
}
 
static void chase(uint32_t c) {
  for(uint16_t i=0; i<rings.numPixels()+4; i++) {
      rings.setPixelColor(i  , c); // Draw new pixel
      rings.setPixelColor(i-4, 0); // Erase pixel a few steps back
      rings.show();
      delay(25);
  }
}