#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h> // Include the adafruit Neopixel Library
#include <MQTTClient.h>

const char ssid[] = "ssid";
const char pass[] = "password";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect();  // <- predefine connect() for setup()


#define PIN         2
#define LED_COUNT  52

const int brightness = 5;

enum mode {modeColorWipe, modeRainbowRain, modeRain, modeRainbow, modeRainbowSparkle, modeSnake, modeSparkle};
mode currentMode = modeRainbowSparkle;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel rings = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRBW + NEO_KHZ800);


void setup() {
  rings.begin();
  rings.setBrightness(brightness);
  rings.show();
  
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  //
  // MQTT brokers usually use port 8883 for secure connections.
  client.begin(IP_ADDRESS, PORT, net);
  client.onMessage(messageReceived);

  connect();
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("led-necklace-huzzah", "key", "token")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
  client.subscribe("lights");
}

void loop() {
  switch(currentMode)
  {
    case modeColorWipe:
      Serial.print("colorWipe\n");
      colorWipe(rings.Color(0, 0, 0, 255), 50); // White
      break;
    case modeRainbow:
      Serial.print("rainbow\n");
      rainbow();
      break;
    case modeRain:
      Serial.print("rain\n");
      rain();
      break;
    case modeRainbowRain:
      Serial.print("rainbow rain\n");
      rainbowRain();
      break;
    case modeSparkle:
      Serial.print("sparkle\n");
      sparkle();
      break;
    case modeSnake:
      Serial.print("snake\n");
      snakeLoop();
      break;
    case modeRainbowSparkle:
      Serial.print("rainbow sparkle\n");
      rainbowSparkle();
      break;
    default:
      break;
  }
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  trigger(payload.c_str());
}

void trigger(const char* event) {
  if(strcmp(event, "rainbow") == 0) {
    Serial.printf("trigger event %s\n", event);
    currentMode = modeRainbow;
  } else if (strcmp(event, "colorWipe") == 0){
     Serial.printf("trigger event %s\n", event);
     currentMode = modeColorWipe;
  } else if (strcmp(event, "rainbowRain") == 0){
     Serial.printf("trigger event %s\n", event);
     currentMode = modeRainbowRain;
  } else if (strcmp(event, "rain") == 0){
     Serial.printf("trigger event %s\n", event);
     currentMode = modeRain;
  } else if (strcmp(event, "sparkle") == 0){
     Serial.printf("trigger event %s\n", event);
     currentMode = modeSparkle;
  } else if (strcmp(event, "snake") == 0){
   Serial.printf("trigger event %s\n", event);
   currentMode = modeSnake;
  } else if (strcmp(event, "rainbowSparkle") == 0){
    Serial.printf("trigger event %s\n", event);
    currentMode = modeRainbowSparkle;
  }
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<rings.numPixels(); i++) {
      rings.setPixelColor(i, c);
      rings.show();
      delay(wait);
  }
}

// These are the pixels in order of animation-- 26 pixels in total:
int sine[] = {
   8, 9, 10, 11, 0, 1, 2, 3, 15, 16, 17, 18, 19, 49, 50, 51, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37 };

 void snake() {
    for(int i=0; i<26; i++) {
    rings.setPixelColor(sine[i], rings.Color(75, 250, 100, 0)); // Draw 'head' pixel
    rings.setPixelColor(sine[(i + 26 - 6) % 26], 0); // Erase 'tail'
    rings.show();
    delay(40);
  }
 }

// Array of pixels in order of animation - 52 in total:
int sineLoop[] = {
   8, 9, 10, 11, 0, 1, 2, 3, 15, 16, 17, 18, 19, 49, 50, 51, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 
  46, 47, 48, 20, 21, 22, 23, 24, 25, 26, 27, 12, 13, 14, 4, 5, 6, 7};
 
void snakeLoop() {
  for(int i=0; i<52; i++) {
    rings.setPixelColor(sineLoop[i], 0); // Erase 'tail'
    rings.setPixelColor(sineLoop[(i + 10) % 52], rings.Color(75, 250, 100, 0)); // Draw 'head' pixel
    rings.show();
    delay(60);
  }
}

// MR SPARKLE PROGRAM!!!!
// Call createSparkle with white color
void sparkle() {
  createSparkle(rings.Color(255, 255, 255));
}

// Call createSparkle with random color
void rainbowSparkle() {
  createSparkle(rings.Color(random(255), random(255), random(255)));
}

// Draw 2 random leds, dim all leds, repeat
void createSparkle(uint32_t color) {
  delay(80);
  rings.setPixelColor(random(52), color);
  rings.setPixelColor(random(52), color);

  for (int x=0; x<52; x++) {
    rings.setPixelColor(x, DimColor(rings.getPixelColor(x), .80));  

  }
  rings.show();
}

uint32_t colors[52];
int ring1[6] = {7,  8,  9, 10, 11,  0};
int ring2[6] = {6,  5,  4,  3,  2,  1};
int ring3[8] = {16, 15, 14, 13, 12, 27, 26, 25};
int ring4[8] = {17, 18, 19, 20, 21, 22, 23, 24};
int ring5[12] = {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
int ring6[12] = {39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28};

// Rain Program
void rain() {
  delay(30);
  uint32_t color = rings.Color(0, 0, 255, 0);
  createRain(color);
}

// Rainbow Rain Program
void rainbowRain() {
  delay(25);
  uint32_t color = rings.Color(random(255), random(255), random(255), 0);
  createRain(color);
}

// Base Rain Program
void createRain(uint32_t color) {

   //first move any ON lights down one on each strip
   for(int x=5; x > 0 ; x--) {
    //if (colors[ring1[x-1]] != 0) {
       colors[ring1[x]] = colors[ring1[x-1]];
       colors[ring1[x-1]] = 0;
     //}
   }
  // each row: special case turn off all first lights
  colors[ring1[0]] = 0;

  //first move any ON lights down one on each strip
   for(int x=5; x > 0 ; x--) {
    //if (colors[ring2[x-1]] != 0) {
       colors[ring2[x]] = colors[ring2[x-1]];
       colors[ring2[x-1]] = 0;
     //}
   }
  // each row: special case turn off all first lights
  colors[ring2[0]] = 0;

  //first move any ON lights down one on each strip
   for(int x=7; x > 0 ; x--) {
    if (colors[ring3[x-1]] != 0) {
       colors[ring3[x]] = colors[ring3[x-1]];
       colors[ring3[x-1]] = DimColor(colors[ring3[x-1]], .40);
     }
   }
  // each row: special case turn off all first lights
  colors[ring3[0]] = 0;

  //first move any ON lights down one on each strip
   for(int x=7; x > 0 ; x--) {
    if (colors[ring4[x-1]] != 0) {
       colors[ring4[x]] = colors[ring4[x-1]];
       colors[ring4[x-1]] = DimColor(colors[ring4[x-1]], .40);
     }
   }
  // each row: special case turn off all first lights
  colors[ring4[0]] = 0;

  //first move any ON lights down one on each strip
   for(int x=11; x > 0 ; x--) {
    if (colors[ring5[x-1]] != 0) {
       colors[ring5[x]] = colors[ring5[x-1]];
       colors[ring5[x-1]] = DimColor(colors[ring5[x-1]], .50);
     }
   }
  // each row: special case turn off all first lights
  colors[ring5[0]] = 0;

  //first move any ON lights down one on each strip
   for(int x=11; x > 0 ; x--) {
    if (colors[ring6[x-1]] != 0) {
       colors[ring6[x]] = colors[ring6[x-1]];
       colors[ring6[x-1]] = DimColor(colors[ring6[x-1]], .50);
     }
   }
  // each row: special case turn off all first lights
  colors[ring6[0]] = 0;
  
  // turn on light at first position of random strip
  int randomOn = random(2);
  if (randomOn == 1) {
    int strip = random(10);
    switch(strip){
      case 0:
        colors[ring1[0]] = color;
      break;
      case 1:
        colors[ring2[0]] = color;
      break;
      case 2:
      case 3:
        colors[ring3[0]] = color;
      break;
      case 4:
      case 5:
        colors[ring4[0]] = color;
      break;
      case 6:
      case 7:
        colors[ring5[0]] = color;
      break;
      case 8:
      case 9:
        colors[ring6[0]] = color;
      break;
      default:
      break;
    }
  }

  updateRings();
}

//Rainbow Program
void rainbow() {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<rings.numPixels(); i++) {
      rings.setPixelColor(i, Wheel((i+j) & 255));
    }
    rings.show();
    delay(20);
  }
}

// draw lights according to strips[][] array
void updateRings() {
  for(int i=0; i<52; i++) {
    rings.setPixelColor(i, colors[i]);
    rings.show();
  }  
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(int WheelPos) {
  if(WheelPos < 85) {
    return rings.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return rings.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return rings.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color)
{
    return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
    return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
    return color & 0xFF;
}

// Return color, dimmed by percentage
uint32_t DimColor(uint32_t color, float dimPercent)
{
  int red = Red(color) * dimPercent;
  int blue = Blue(color) * dimPercent;
  int green = Green(color) * dimPercent;
  uint32_t dimColor = rings.Color(red, green, blue);
  return dimColor;
}
