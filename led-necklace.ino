#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h> // Include the adafruit Neopixel Library
#include <MQTTClient.h>
#include "arduino_secrets.h" 

#define PIN         2
#define LED_COUNT  52

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

unsigned long lastMillis = 0;
const int brightness = 15;
bool rainbowOn = false;
int colorR = 0;
int colorG = 0;
int colorB = 255;
uint16_t TotalSteps = 400;  // total number of steps in the pattern
uint16_t Index;  // current step within the pattern
uint32_t Color1, Color2;  // What colors are in use (used by Fade)

WiFiClient net;
MQTTClient client;

void connect();  // <- predefine connect() for setup()

enum mode {modeFade, modeRain, modeSnake, modeSparkle};
mode currentMode = modeRain;

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
  while (!client.connect("led-necklace-m0", "", "")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
  client.subscribe("lights");
}

void messageReceived(String &topic, String &payload) {
  const char* delimiter = ",";
  String incomingMode = payload.substring(0,payload.indexOf(delimiter));
  String colorValue   = payload.substring(incomingMode.length()+2,payload.length());
  Serial.println("topic: " + topic);
  Serial.println("payload: " + incomingMode);
  
  if(colorValue == "rainbow") {
    rainbowOn = true;
    colorR = 0;
    colorG = 0;
    colorB = 0;
    Serial.println("color: " + colorValue);
  }
  else {
    rainbowOn = false;  
    // rainbow not used, get color values
    String colorStringR = colorValue.substring(0, colorValue.indexOf(delimiter));
    colorValue.remove(0,colorStringR.length()+2);
    String colorStringG = colorValue.substring(0, colorValue.indexOf(delimiter));
    colorValue.remove(0,colorStringG.length()+2);
    String colorStringB = colorValue.substring(0, colorValue.indexOf(delimiter));
    Serial.println("colorR: " + colorStringR);
    Serial.println("colorG: " + colorStringG);
    Serial.println("colorB: " + colorStringB);
    colorR = colorStringR.toInt();
    colorG = colorStringG.toInt();
    colorB = colorStringB.toInt();
  }
  trigger(incomingMode.c_str());
}

void trigger(const char* event) {
  if (strcmp(event, "fade") == 0){
     currentMode = modeFade;
  } else if (strcmp(event, "rain") == 0){
     currentMode = modeRain;
  } else if (strcmp(event, "sparkle") == 0){
     currentMode = modeSparkle;
  } else if (strcmp(event, "snake") == 0){
   currentMode = modeSnake;
  }
}

void loop() {
  switch(currentMode)
  {
    case modeFade:
      Serial.print("fade\n");
      runFade(10);
      break;
     case modeRain:
      Serial.print("rain\n");
      rain(25);
      break;
    case modeSparkle:
      Serial.print("sparkle\n");
      sparkle(80);
      break;
    case modeSnake:
      Serial.print("snake\n");
      snakeLoop(50);
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

// Fill the dots one after the other with a color
void runFade(uint8_t wait) {
  if(rainbowOn == true) {
    uint16_t i, j;
    for(j=0; j<256; j++) {
      for(i=0; i<rings.numPixels(); i++) {
        rings.setPixelColor(i, Wheel((i+j) & 255));
      }
      rings.show();
      delay(wait);
    }
  }
  else {
    Index = 0;
    Color1 = rings.Color(colorR, colorG, colorB);
    Color2 = rings.Color(1,1,1);
    while(Index+50 <= TotalSteps) {
      fadeCycle();  // Fading darker
      Index++;
    }
    while(Index > 50) {
      fadeCycle();  // Fading brighter
      Index--;
    }
  }
}
void fadeCycle() {
  uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
  uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
  uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
  for(uint16_t i=0; i<rings.numPixels(); i++) {
    rings.setPixelColor(i, rings.Color(red, green, blue));        
  }
  rings.show();
}

// Array of pixels in order of animation - 52 in total:
int sineLoop[] = {
   8, 9, 10, 11, 0, 1, 2, 3, 15, 16, 17, 18, 19, 49, 50, 51, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 
  46, 47, 48, 20, 21, 22, 23, 24, 25, 26, 27, 12, 13, 14, 4, 5, 6, 7};
  
void snakeLoop(uint8_t wait) {
  uint32_t color = rings.Color(colorR, colorG, colorB);
  if (rainbowOn == true) {
      for(uint16_t i=0; i<rings.numPixels(); i++) {
        rings.setPixelColor(sineLoop[i], 0); // Erase 'tail'
        rings.setPixelColor(sineLoop[(i + 10) % 52], Wheel(Index));
        Index++;
        if(Index > 255) {
          Index = 0;
        }
        rings.show();
        delay(wait);
      }
  }
  else {
    for(int i=0; i<52; i++) {
      rings.setPixelColor(sineLoop[i], 0); // Erase 'tail'
      rings.setPixelColor(sineLoop[(i + 10) % 52], color); // Draw 'head' pixel
      rings.show();
      delay(wait);
    }
  }
}

// Draw 2 random leds, dim all leds, repeat
void sparkle(uint8_t wait) {
  uint32_t color;
  if (rainbowOn == true) {
    color = rings.Color(random(255), random(255), random(255));
  }
  else {
    color = rings.Color(colorR, colorG, colorB);
  }
  rings.setPixelColor(random(52), color);
  rings.setPixelColor(random(52), color);

  for (int x=0; x<52; x++) {
    rings.setPixelColor(x, DimColor(rings.getPixelColor(x), .80));  
  }
  rings.show();
  delay(wait);
}
uint32_t colors[52];
int ring1[6] = {7,  8,  9, 10, 11,  0};
int ring2[6] = {6,  5,  4,  3,  2,  1};
int ring3[8] = {16, 15, 14, 13, 12, 27, 26, 25};
int ring4[8] = {17, 18, 19, 20, 21, 22, 23, 24};
int ring5[12] = {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
int ring6[12] = {39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28};

// Moves a raindrop down 1 step
void stepRaindrop(int arrayLength, int ringArray[], float fadeAmount) {
  //first move any ON lights down one on each strip
   for(int x=arrayLength; x > 0 ; x--) {
    if (colors[ringArray[x-1]] != 0) {
       colors[ringArray[x]] = colors[ringArray[x-1]];
       colors[ringArray[x-1]] = DimColor(colors[ringArray[x-1]], fadeAmount);
    }
   }
  // each row: special case, turn off all first lights
  colors[ringArray[0]] = 0;  
}

// Rain Program
void rain(uint8_t wait) {
  uint32_t color;
  if (rainbowOn == true) {
    color = rings.Color(random(255), random(255), random(255));
  }
  else {
    color = rings.Color(colorR, colorG, colorB);
  }
  stepRaindrop(5,ring1, .30);
  stepRaindrop(5,ring2, .30);
  stepRaindrop(7,ring3, .40);
  stepRaindrop(7,ring4, .40);
  stepRaindrop(13,ring5, .50);
  stepRaindrop(13,ring6, .50);
  
  // turn on light at first position of random strip
  // weighted a bit towards the bigger rings
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
  delay(wait);
}

// udpate all lights according to colors[] array
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
uint8_t Red(uint32_t color){
    return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color){
    return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color){
    return color & 0xFF;
}

// Return color, dimmed by percentage
uint32_t DimColor(uint32_t color, float dimPercent){
  int red = Red(color) * dimPercent;
  int blue = Blue(color) * dimPercent;
  int green = Green(color) * dimPercent;
  uint32_t dimColor = rings.Color(red, green, blue);
  return dimColor;
}

//DEBUG - Determine pixel positions
//rings.setPixelColor(0, rings.Color(255, 0, 0));
//rings.setPixelColor(12, rings.Color(0, 255, 0));
//rings.setPixelColor(23, rings.Color(0, 0, 255));
//rings.setPixelColor(24, rings.Color(255, 0, 0));
//rings.setPixelColor(31, rings.Color(0, 255, 0));
//rings.setPixelColor(39, rings.Color(0, 0, 255));
//rings.setPixelColor(40, rings.Color(255, 0, 0));
//rings.setPixelColor(45, rings.Color(0, 255, 0));
//rings.setPixelColor(51, rings.Color(0, 0, 255));
//rings.show();
