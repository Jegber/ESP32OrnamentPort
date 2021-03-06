#include <Arduino.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "FastLED.h"
FASTLED_USING_NAMESPACE
#include <FS.h>
#include "SPIFFS.h"
#include <EEPROM.h>
#include "GradientPalettes.h"

// Access Point Mode and password
const bool apMode = false;
const char WiFiAPPSK[] = "";

// Wi-Fi network to connect to (if not in Access Point mode, above)
const char* ssid = "Fine Mustard";
const char* password = "dijonwedding16";

AsyncWebServer server(80);

#define DATA_PIN 2     // Data pin. use pin as labeled on board
#define LED_TYPE WS2812 // Using the ws2812 LEDs
#define COLOR_ORDER GRB // Colors are sent in Green-Red-Blue ordering
#define NUM_LEDS 20     // How many leds are in the strip

#define MILLI_AMPS 1000 // IMPORTANT: set here the max milli-Amps of your power supply 5V 2A = 2000
#define FRAMES_PER_SECOND  120  // Refresh speed. With the Access Point / Web Server the animations run a bit slower.

CRGB leds[NUM_LEDS];  // Set up the LED array

uint8_t patternIndex = 0;

// Brightness levels
const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
int brightnessIndex = 0;
uint8_t brightness = brightnessMap[brightnessIndex];

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
#define SECONDS_PER_PALETTE 10

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

uint8_t currentPatternIndex = 0; // Index number of which pattern is current
bool autoplayEnabled = false;

uint8_t autoPlayDurationSeconds = 10;
unsigned int autoPlayTimeout = 0;

uint8_t currentPaletteIndex = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

CRGB solidColor = CRGB::Blue;
CRGB timerColor = CRGB::Green;

uint8_t power = 1;
uint8_t timer = 1;
uint8_t defaultTime = 100; //Default max time in a day in seconds
uint32_t currentScreenTime = 0; //This counts the amount of screen time in a day in seconds
uint32_t screenTimeMax = defaultTime; //This is the daily screen time threshold in seconds


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void loadSettings();
void sendAll(AsyncWebServerRequest *request);
void sendPower(AsyncWebServerRequest *request);
void sendTimer(AsyncWebServerRequest *request);
void sendTimerMax(AsyncWebServerRequest *request);
void sendPattern(AsyncWebServerRequest *request);
void sendPalette(AsyncWebServerRequest *request);
void sendBrightness(AsyncWebServerRequest *request);
void sendSolidColor(AsyncWebServerRequest *request);
void setPower(uint8_t value);
void setTimer(uint8_t value);
void setTimerMax(uint32_t value);
void setSolidColor(CRGB color);
void setSolidColor(uint8_t r, uint8_t g, uint8_t b);
void adjustPattern(bool up);
void setPattern(int value);
void setPalette(int value);
void adjustBrightness(bool up);
void setBrightness(int value);
void showSolidColor();
void timerGreenRed();
void rainbow();
void rainbowWithGlitter();
void addGlitter( fract8 chanceOfGlitter);
void confetti();
void sinelon();
void bpm();
void juggle();
void pride();
void colorwaves();
void palettetest();


void setup(void) {
  Serial.begin(115200);
  delay(100);
  Serial.setDebugOutput(true);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // for WS2812 (Neopixel)
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, solidColor);
  //FastLED.show();

  EEPROM.begin(512);
  loadSettings();

  FastLED.setBrightness(brightness);

  Serial.println();
  Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size());
  Serial.print( F("SDK: ") ); Serial.println(system_get_sdk_version());
  Serial.print( F("Flash Size: ") ); Serial.println(spi_flash_get_chip_size());
  Serial.println();


  SPIFFS.begin();
  listDir(SPIFFS, "/", 2);


  // If we are to set up the the board in Access Point mode
  if (apMode)
  {
    WiFi.mode(WIFI_AP);

    // Do a little work to get a unique-ish name. Append the
    // last two bytes of the MAC (HEX'd) to "Thing-":
    //uint8_t mac[WL_MAC_ADDR_LENGTH];
    //WiFi.softAPmacAddress(mac);
    /*
    String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                   String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
    macID.toUpperCase();
    */
    String AP_NameString = "ESP32Yippee";

    char AP_NameChar[AP_NameString.length() + 1];
    memset(AP_NameChar, 0, AP_NameString.length() + 1);

    for (int i = 0; i < AP_NameString.length(); i++)
      AP_NameChar[i] = AP_NameString.charAt(i);

    WiFi.softAP(AP_NameChar, WiFiAPPSK);

    Serial.printf("Connect to Wi-Fi access point: %s\n", AP_NameChar);
    Serial.println("and open http://192.168.4.1 in your browser");
  }
  // If we are to set up the board as a client on a LAN
  else
  {
    WiFi.mode(WIFI_STA);
    Serial.printf("Connecting to %s\n", ssid);
    if (String(WiFi.SSID()) != String(ssid)) {
      WiFi.begin(ssid, password);
    }

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.print("Connected! Open http://");
    Serial.print(WiFi.localIP());
    Serial.println(" in your browser");
  }


  server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendAll(request);
  });

  server.on("/power", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Power is being fiddled with\n");
      sendPower(request);
  });

  server.on("/power", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.print("/power POST\n");
      String value = request->arg("value");
      setPower(value.toInt());
      sendPower(request);
  });

  server.on("/timer", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendTimer(request);
  });


  server.on("/timer", HTTP_POST, [](AsyncWebServerRequest *request) {
    String value = request->arg("value");
    setTimer(value.toInt());
    sendTimer(request);
  });


  server.on("/timerMax", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendTimerMax(request); //Still maybe need to update this
  });


  server.on("/timerMax", HTTP_POST, [](AsyncWebServerRequest *request) {
    String hours = request->arg("hour");
    String minutes = request->arg("min");
    uint32_t totalTimeSeconds = hours.toInt()*3600+minutes.toInt()*60;
    setTimerMax(totalTimeSeconds);
    sendTimerMax(request); //Still maybe need to update this
  });


  server.on("/solidColor", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendSolidColor(request);
  });


  server.on("/solidColor", HTTP_POST, [](AsyncWebServerRequest *request) {
    String r = request->arg("r");
    String g = request->arg("g");
    String b = request->arg("b");
    setSolidColor(r.toInt(), g.toInt(), b.toInt());
    sendSolidColor(request);
  });


  server.on("/pattern", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendPattern(request);
  });


  server.on("/pattern", HTTP_POST, [](AsyncWebServerRequest *request) {
    String value = request->arg("value");
    setPattern(value.toInt());
    sendPattern(request);
  });


  server.on("/patternUp", HTTP_POST, [](AsyncWebServerRequest *request) {
    adjustPattern(true);
    sendPattern(request);
  });

  server.on("/patternDown", HTTP_POST, [](AsyncWebServerRequest *request) {
    adjustPattern(false);
    sendPattern(request);
  });

  server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendBrightness(request);
  });


  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    String value = request->arg("value");
    setBrightness(value.toInt());
    sendBrightness(request);
  });


  server.on("/brightnessUp", HTTP_POST, [](AsyncWebServerRequest *request) {
    adjustBrightness(true);
    sendBrightness(request);
  });

  server.on("/brightnessDown", HTTP_POST, [](AsyncWebServerRequest *request) {
    adjustBrightness(false);
    sendBrightness(request);
  });

  server.on("/palette", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendPalette(request);
  });


  server.on("/palette", HTTP_POST, [](AsyncWebServerRequest *request) {
    String value = request->arg("value");
    setPalette(value.toInt());
    sendPalette(request);
  });


  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
  server.begin();
  Serial.println("HTTP server started");

  autoPlayTimeout = millis() + (autoPlayDurationSeconds * 1000);
}

typedef void (*Pattern)();
typedef struct {
  Pattern pattern;
  String name;
} PatternAndName;
typedef PatternAndName PatternAndNameList[];

// List of patterns to cycle through.  Each is defined as a separate function below.
PatternAndNameList patterns = {
  { colorwaves, "Color Waves" },
  { palettetest, "Palette Test" },
  { pride, "Pride" },
  { rainbow, "Rainbow" },
  { rainbowWithGlitter, "Rainbow With Glitter" },
  { confetti, "Confetti" },
  { sinelon, "Sinelon" },
  { juggle, "Juggle" },
  { bpm, "BPM" },
  { showSolidColor, "Solid Color" },
  { timerGreenRed, "Timer Colors"}
};

const uint8_t patternCount = ARRAY_SIZE(patterns);

typedef struct {
  CRGBPalette16 palette;
  String name;
} PaletteAndName;
typedef PaletteAndName PaletteAndNameList[];

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

const uint8_t paletteCount = ARRAY_SIZE(palettes);

const String paletteNames[paletteCount] = {
  "Rainbow",
  "Rainbow Stripe",
  "Cloud",
  "Lava",
  "Ocean",
  "Forest",
  "Party",
  "Heat"
};

void loop(void) {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  //server.handleClient();

  //  handleIrInput();

  if (power == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(15);
    return;
  }
  if (timer == 0) {
      currentScreenTime = 0;
  }
  EVERY_N_SECONDS(1){
    if(currentScreenTime < screenTimeMax){
      currentScreenTime++;
    }
    //red go up, green go down;
    uint8_t maxColor = 255;
    uint32_t halfTime = screenTimeMax/2;
    if(currentScreenTime>=screenTimeMax){
      uint8_t timerRed = maxColor;
      timerColor = CRGB(timerRed,0,0); //Failsafe for if change max total time to below the current time
    }
    else if(((screenTimeMax-currentScreenTime)/halfTime)){
      //Still on green side
      uint32_t timerOver = (screenTimeMax-currentScreenTime)%halfTime;
      uint8_t timerGreen = maxColor;
      uint8_t timerRed = maxColor*(halfTime-timerOver)/halfTime;
      timerColor = CRGB(timerRed,timerGreen,0);
    }
    else{
      //More on Red side
      uint8_t timerRed = maxColor;
      uint8_t timerGreen = maxColor*(screenTimeMax-currentScreenTime)/halfTime;
      timerColor = CRGB(timerRed,timerGreen,0);

    }
  }
  // EVERY_N_SECONDS(10) {
  //   Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size());
  // }

  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  // slowly blend the current cpt-city gradient palette to the next
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
  }

  if (autoplayEnabled && millis() > autoPlayTimeout) {
    adjustPattern(true);
    autoPlayTimeout = millis() + (autoPlayDurationSeconds * 1000);
  }

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex].pattern();

  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void loadSettings()
{
  brightness = EEPROM.read(0);

  currentPatternIndex = EEPROM.read(1);
  if (currentPatternIndex < 0)
    currentPatternIndex = 0;
  else if (currentPatternIndex >= patternCount)
    currentPatternIndex = patternCount - 1;

  byte r = EEPROM.read(2);
  byte g = EEPROM.read(3);
  byte b = EEPROM.read(4);

  if (r == 0 && g == 0 && b == 0)
  {
  }
  else
  {
    solidColor = CRGB(r, g, b);
  }

  currentPaletteIndex = EEPROM.read(5);
  if (currentPaletteIndex < 0)
    currentPaletteIndex = 0;
  else if (currentPaletteIndex >= paletteCount)
    currentPaletteIndex = paletteCount - 1;
}

void sendAll(AsyncWebServerRequest* request)
{
  //Serial.println("sendAll()\n");
  String json = "{";

  json += "\"power\":" + String(power) + ",";
  json += "\"timer\":" + String(timer) + ",";
  json += "\"timerMax\":" + String(screenTimeMax) + ",";
  json += "\"brightness\":" + String(brightness) + ",";

  json += "\"currentPattern\":{";
  json += "\"index\":" + String(currentPatternIndex);
  json += ",\"name\":\"" + patterns[currentPatternIndex].name + "\"}";

  json += ",\"currentPalette\":{";
  json += "\"index\":" + String(currentPaletteIndex);
  json += ",\"name\":\"" + paletteNames[currentPaletteIndex] + "\"}";

  json += ",\"solidColor\":{";
  json += "\"r\":" + String(solidColor.r);
  json += ",\"g\":" + String(solidColor.g);
  json += ",\"b\":" + String(solidColor.b);
  json += "}";

  json += ",\"patterns\":[";
  for (uint8_t i = 0; i < patternCount; i++)
  {
    json += "\"" + patterns[i].name + "\"";
    if (i < patternCount - 1)
      json += ",";
  }
  json += "]";

  json += ",\"palettes\":[";
  for (uint8_t i = 0; i < paletteCount; i++)
  {
    json += "\"" + paletteNames[i] + "\"";
    if (i < paletteCount - 1)
      json += ",";
  }
  json += "]";

  json += "}";

  request->send(200, "text/json", json);
  json = String();
}

void sendPower(AsyncWebServerRequest *request)
{
  Serial.print("sendPower()\n");
  String json = String(power);
  request->send(200, "text/json", json);
  json = String();
}

void sendTimer(AsyncWebServerRequest *request)
{
    Serial.print("sendTimer()\n");
    String json = String(timer);
    request->send(200, "text/json", json);
    json = String();
}

void sendTimerMax(AsyncWebServerRequest *request) //Do I need to update this to hours minutes??
{
    Serial.print("sendTimerMax()\n");
  String json = String(screenTimeMax);
  request->send(200, "text/json", json);
  json = String();
}

void sendPattern(AsyncWebServerRequest *request)
{
    Serial.print("sendPattern()\n");
  String json = "{";
  json += "\"index\":" + String(currentPatternIndex);
  json += ",\"name\":\"" + patterns[currentPatternIndex].name + "\"";
  json += "}";
  request->send(200, "text/json", json);
  json = String();
}

void sendPalette(AsyncWebServerRequest *request)
{
    Serial.print("sendPalette()\n");
  String json = "{";
  json += "\"index\":" + String(currentPaletteIndex);
  json += ",\"name\":\"" + paletteNames[currentPaletteIndex] + "\"";
  json += "}";
  request->send(200, "text/json", json);
  json = String();
}

void sendBrightness(AsyncWebServerRequest *request)
{
    Serial.print("sendBrightness()\n");
  String json = String(brightness);
  request->send(200, "text/json", json);
  json = String();
}

void sendSolidColor(AsyncWebServerRequest *request)
{
    Serial.print("sendSolidColor()\n");
  String json = "{";
  json += "\"r\":" + String(solidColor.r);
  json += ",\"g\":" + String(solidColor.g);
  json += ",\"b\":" + String(solidColor.b);
  json += "}";
  request->send(200, "text/json", json);
  json = String();
}

void setPower(uint8_t value)
{
    printf("Setting power to:%d\n", value);
    power = value == 0 ? 0 : 1;
}

void setTimer(uint8_t value)
{
  timer = value == 0 ? 0 : 1;
}
void setTimerMax(uint32_t value)
{
  screenTimeMax = value;
}

void setSolidColor(CRGB color)
{
  setSolidColor(color.r, color.g, color.b);
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  solidColor = CRGB(r, g, b);

  EEPROM.write(2, r);
  EEPROM.write(3, g);
  EEPROM.write(4, b);

  setPattern(patternCount - 1);
}

// increase or decrease the current pattern number, and wrap around at the ends
void adjustPattern(bool up)
{
  if (up)
    currentPatternIndex++;
  else
    currentPatternIndex--;

  // wrap around at the ends
  if (currentPatternIndex < 0)
    currentPatternIndex = patternCount - 1;
  if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;

  if (autoplayEnabled) {
    EEPROM.write(1, currentPatternIndex);
    EEPROM.commit();
  }
}

void setPattern(int value)
{
  // don't wrap around at the ends
  if (value < 0)
    value = 0;
  else if (value >= patternCount)
    value = patternCount - 1;

  currentPatternIndex = value;

  if (autoplayEnabled == 0) {
    EEPROM.write(1, currentPatternIndex);
    EEPROM.commit();
  }
}

void setPalette(int value)
{
  // don't wrap around at the ends
  if (value < 0)
    value = 0;
  else if (value >= paletteCount)
    value = paletteCount - 1;

  currentPaletteIndex = value;

  EEPROM.write(5, currentPaletteIndex);
  EEPROM.commit();
}

// adjust the brightness, and wrap around at the ends
void adjustBrightness(bool up)
{
  if (up)
    brightnessIndex++;
  else
    brightnessIndex--;

  // wrap around at the ends
  if (brightnessIndex < 0)
    brightnessIndex = brightnessCount - 1;
  else if (brightnessIndex >= brightnessCount)
    brightnessIndex = 0;

  brightness = brightnessMap[brightnessIndex];

  FastLED.setBrightness(brightness);

  EEPROM.write(0, brightness);
  EEPROM.commit();
}

void setBrightness(int value)
{
  // don't wrap around at the ends
  if (value > 255)
    value = 255;
  else if (value < 0) value = 0;

  brightness = value;

  FastLED.setBrightness(brightness);

  EEPROM.write(0, brightness);
  EEPROM.commit();
}

void showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);
}

void timerGreenRed()
{
  fill_solid(leds, NUM_LEDS, timerColor);
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 10);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  //  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  //  leds[pos] += CHSV( gHue, 255, 192);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle()
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for ( int i = 0; i < 8; i++)
  {
    //    leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
    leds[beatsin16(i + 7, 0, NUM_LEDS)] |= ColorFromPalette(palettes[currentPaletteIndex], dothue);
    dothue += 32;
  }
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    nblend(leds[i], newcolor, 64);
  }
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette(gCurrentPalette, index, bri8);

    nblend(leds[i], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void palettetest()
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( leds, NUM_LEDS, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}
