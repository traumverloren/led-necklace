#include <ESP8266WiFi.h> // Include the ESP8266 Library
#include <Adafruit_NeoPixel.h> // Include the adafruit Neopixel Library
#include <WebSocketsClient.h> // Include Socket.IO client library to communicate with Server!

const char* ssid     = "ssid";
const char* password = "pw";

#define PIN         2
#define LED_COUNT  52
const int brightness = 30;

enum mode {modeColorWipe, modeRainbowRain, modeRain, modeRainbow, modeSocketConnect, modeSparkle};
mode currentMode = modeSocketConnect;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel rings = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRBW + NEO_KHZ800);

WebSocketsClient webSocket;

#define MESSAGE_INTERVAL 10000
#define HEARTBEAT_INTERVAL 5000

uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t eventLength) {
    String msg;
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            isConnected = false;
            currentMode = modeSocketConnect;
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n",  payload);
            // send message to server when Connected
            // socket.io upgrade confirmation message (required)
            webSocket.sendTXT("5");
            isConnected = true;
            currentMode = modeSparkle;
            break;
        case WStype_TEXT:
            msg = String((char*)payload);
            if(msg.startsWith("42")) {
              trigger(getEventName(msg).c_str(), getEventPayload(msg).c_str(), eventLength);
            }
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", eventLength);
            hexdump(payload, eventLength);

            // send data to server
            // webSocket.sendBIN(payload, eventLength);
            break;
    }
}

const String getEventName(const String msg) {
  return msg.substring(4, msg.indexOf("\"",4));
}

const String getEventPayload(const String msg) {
  String result = msg.substring(msg.indexOf("\"",4)+2,msg.length()-1);
  if(result.startsWith("\"")) {
    result.remove(0,1);
  }
  if(result.endsWith("\"")) {
    result.remove(result.length()-1);
  }
  return result;
}

void trigger(const char* event, const char * payload, size_t triggerLength) {
  Serial.printf("[WSc] trigger event %s\n", event);
  
  if(strcmp(event, "rainbow") == 0) {
    Serial.printf("[WSc] trigger event %s\n", event);
    currentMode = modeRainbow;

  } else if (strcmp(event, "colorWipe") == 0){
     Serial.printf("[WSc] trigger event %s\n", event);
     currentMode = modeColorWipe;
  } else if (strcmp(event, "rainbowRain") == 0){
     Serial.printf("[WSc] trigger event %s\n", event);
     currentMode = modeRainbowRain;
  } else if (strcmp(event, "rain") == 0){
     Serial.printf("[WSc] trigger event %s\n", event);
     currentMode = modeRain;
  } else if (strcmp(event, "sparkle") == 0){
     Serial.printf("[WSc] trigger event %s\n", event);
     currentMode = modeSparkle;
  }
}

void setup() {  
  rings.begin();
  rings.setBrightness(brightness);
  rings.show();
  
  Serial.begin(115200); // Get ready for serial communications and display the connection status
  Serial.print("Connecting to WiFi network -  ");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!!!!!!!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(10);
  webSocket.beginSocketIO("led-umbrella.herokuapp.com", 80);
  isConnected = true;   
  webSocket.onEvent(webSocketEvent);
  delay(500);
}

void loop() {
  Serial.printf("loop heap size: %u\n", ESP.getFreeHeap());
  webSocket.loop();
  
  switch(currentMode)
  {
    case modeColorWipe:
      Serial.print("colorWipe\n");
      colorWipe(rings.Color(0, 0, 0, 255), 50); // White
      break;
    case modeRainbow:
      Serial.print("rainbow\n");
      rainbow(0);
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
    default:
      break;
  }

  if(isConnected) {

      uint64_t now = millis();

      if(now - messageTimestamp > MESSAGE_INTERVAL) {
        messageTimestamp = now;
        // example socket.io message with type "messageType" and JSON payload
        webSocket.sendTXT("42[\"messageType\",{\"greeting\":\"hello\"}]");
        delay(10);
      }
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
        heartbeatTimestamp = now;
        // socket.io heartbeat message
        webSocket.sendTXT("2");
        delay(10);
      }
   } else {
      Serial.printf("[WSc] Reconnected!\n");
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      delay(10);
      webSocket.beginSocketIO("led-umbrella.herokuapp.com", 80);
      isConnected = true;   
      currentMode = modeSocketConnect;
    }    
}

// draw lights according to strips[][] array
//void updateStrips() {
//  for(int x=0; x < stripCount; x++) {
//    for(int y=ledCount-1; y>=0; y--) {
//      pixelStrips[x].setPixelColor(y, strips[x][y]);
//      pixelStrips[x].show();
//    }
//  }  
//}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<rings.numPixels(); i++) {
      rings.setPixelColor(i, c);
      rings.show();
      delay(wait);
  }
}

// MR SPARKLE PROGRAM!!!!
// Draw a random led, remove a random led.
void sparkle() {
  rings.setPixelColor(random(52), rings.Color(255, 255, 255));

  rings.setPixelColor(random(52), rings.Color(0, 0, 0));  
}

// Rain Program
void rain() {
  delay(10);
  uint32_t color = rings.Color(0, 0, 255);
  createRain(color);
}

// Rainbow Rain Program
void rainbowRain() {
  delay(25);
  uint32_t color = rings.Color(random(255), random(255), random(255));
  createRain(color);
}

// Base Rain Program
void createRain(uint32_t color) {
  // first move any ON lights down one on each strip
  // for(int x=0; x < stripCount; x++) {
  //   for(int y=ledCount-1; y>0; y--) {
  //     if (strips[x][y-1] != 0) {
  //         strips[x][y] = strips[x][y-1];
  //         strips[x][y-1] = DimColor(strips[x][y-1], .50);  
  //     }
  //   }
  //   // each row: special case turn off all first lights
  //   strips[x][0] = 0;
  // }
  // // turn on light at first position of random strip
  // strips[random(stripCount)][0] = color;

  // updateStrips();
}

//Rainbow Program
void rainbow(uint8_t wait) {
  int j, x;
  
  for(j=1; j < 256; j++) {
      for(x=LED_COUNT-1; x>=0; x--) {   
        rings.setPixelColor(x, Wheel(j));
      }
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
