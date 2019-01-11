#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <EEPROM.h>
#include "config.h"

WebServer server(80);

uint8_t gCurrentHair = 1;
uint8_t gCurrentEye = 0;
#ifdef TAIL_LEDS
uint8_t gCurrentTail = 0;
#endif
uint8_t gCurrentOverlay = 0;
uint8_t gHue = 0;   // rotating "base color" used by many of the patterns

#define BRIGHTNESS_ADDR 0x0
#define FPS_ADDR BRIGHTNESS_ADDR + sizeof(brightness)
#define SPEED_ADDR FPS_ADDR + sizeof(fps)
#define HAIR_ADDR SPEED_ADDR + sizeof(speed)
#define EYE_ADDR HAIR_ADDR + sizeof(gCurrentHair)
#define OVERLAY_ADDR EYE_ADDR + sizeof(gCurrentEye)
#ifdef TAIL_LEDS
#define TAIL_ADDR OVERLAY_ADDR + sizeof(gCurrentOverlay)
#endif

CRGB rawleds[NUM_LEDS];
CRGB right_eye[3];
CRGB left_eye[3];
CRGB strip1[STRIP1_LEDS];
CRGB strip2[STRIP2_LEDS];
CRGB strip3[STRIP3_LEDS];
CRGB strip4[STRIP4_LEDS];
#ifdef STRIP5_LEDS
CRGB strip5[STRIP5_LEDS];
#endif
#ifdef TAIL_LEDS
CRGB tail[TAIL_LEDS];
#endif

IPAddress apIP(10, 10, 10, 10);
DNSServer dns_server;

void setup() {
        delay(1000);

        Serial.begin(115200);
        Serial.println();

        setup_led();
        setup_wifi();
        setup_dns();
        setup_server();
        setup_eeprom();
}

void loop() {
        ArduinoOTA.handle();
        dns_server.processNextRequest();
        server.handleClient();

        // Call the current pattern and overlay function once, updating the 'leds' array
        gHairPatterns[gCurrentHair]();
        gEyePatterns[gCurrentEye]();
    #ifdef TAIL_LEDS
        gTailPatterns[gCurrentTail]();
    #endif
        gOverlays[gCurrentOverlay]();

        // merge CRGB arrays
        rawleds[0] = right_eye[2];
        rawleds[1] = right_eye[1];
        rawleds[2] = right_eye[0];
        // memcpy(rawleds, right_eye, sizeof(right_eye));
        memcpy(&rawleds[3], left_eye, sizeof(left_eye));
        memcpy(&rawleds[6], strip1, sizeof(strip1));
        memcpy(&rawleds[6+STRIP1_LEDS], strip2, sizeof(strip2));
        memcpy(&rawleds[6+STRIP1_LEDS+STRIP2_LEDS], strip3, sizeof(strip3));
        memcpy(&rawleds[6+STRIP1_LEDS+STRIP2_LEDS+STRIP3_LEDS], strip4, sizeof(strip4));
    #ifdef STRIP5_LEDS
        memcpy(&rawleds[6+STRIP1_LEDS+STRIP2_LEDS+STRIP3_LEDS+STRIP4_LEDS], strip5, sizeof(strip5));
    #ifdef TAIL_LEDS
        memcpy(&rawleds[6+STRIP1_LEDS+STRIP2_LEDS+STRIP3_LEDS+STRIP4_LEDS+STRIP5_LEDS], tail, sizeof(tail));
    #endif
    #else
    #ifdef TAIL_LEDS
        memcpy(&rawleds[6+STRIP1_LEDS+STRIP2_LEDS+STRIP3_LEDS+STRIP4_LEDS], tail, sizeof(tail));
    #endif
    #endif

        // have eyes and tail at full brightness while dimming down the hair
    #ifdef TAIL_LEDS
        fadeToBlackBy(rawleds + 6, NUM_LEDS - 6 - TAIL_LEDS, 240);
    #else
        fadeToBlackBy(rawleds + 6, NUM_LEDS - 6, 240);
    #endif

        // send the 'leds' array out to the actual LED strip
        FastLED.show();
        // insert a delay to keep the framerate modest
        FastLED.delay(1000/fps);

        // do some periodic updates
        // slowly cycle the "base color" through the rainbow
        EVERY_N_MILLISECONDS( 1 ) {
                gHue -= speed;
        }
        // return to default pattern and no overlay after a few seconds
        // EVERY_N_SECONDS( 4 ) { gCurrentHair = DEFAULT_PATTERN; }
        // EVERY_N_SECONDS( 4 ) { gCurrentOverlay = NO_OVERLAY; }
}

/*
    init functions
 */
void setup_led() {
    #ifdef RACHEL
        FastLED.addLeds<CHIPSET, DATA_PIN>(rawleds, NUM_LEDS);
    #else
        FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(rawleds, NUM_LEDS);
    #endif
        FastLED.setCorrection(TypicalSMD5050);
        FastLED.setBrightness(brightness);
        black_hair();
        black_eyes();
        black_tail();
}

void setup_wifi() {
        Serial.println("Configuring access point...");
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(ssid, password, CHANNEL);

        IPAddress myIP = WiFi.softAPIP();
        Serial.print("IP address:");
        Serial.println(myIP);

        WiFi.printDiag(Serial);

        // Hostname defaults to ESP-[ChipID]
        ArduinoOTA.setHostname(HOSTNAME);

        // Password can be set with it's md5 value as well
        ArduinoOTA.setPasswordHash(OTA_PASSWORD);

        ArduinoOTA.onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                        type = "sketch";
                } else { // U_SPIFFS
                        type = "filesystem";
                }

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
        });
        ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) {
                        Serial.println("Auth Failed");
                } else if (error == OTA_BEGIN_ERROR) {
                        Serial.println("Begin Failed");
                } else if (error == OTA_CONNECT_ERROR) {
                        Serial.println("Connect Failed");
                } else if (error == OTA_RECEIVE_ERROR) {
                        Serial.println("Receive Failed");
                } else if (error == OTA_END_ERROR) {
                        Serial.println("End Failed");
                }
        });
        ArduinoOTA.begin();
}

void setup_dns() {
        dns_server.start(DNS_PORT, hostname, apIP);
}

void setup_server() {
        server.on("/on", http_on);
        server.on("/off", http_off);
        server.on("/hair", http_hair);
        server.on("/hair/off", http_hair_off);
        server.on("/hair/white", http_hair_white);
        server.on("/hair/rainbow", http_hair_rainbow);
        server.on("/eyes", http_eyes);
    #ifdef TAIL_LEDS
        server.on("/tail", http_tail);
    #endif
        server.on("/test", http_test);
        server.on("/overlay", http_overlay);
        server.on("/overlay/off", http_overlay_off);
        server.on("/overlay/glitter", http_overlay_glitter);
        server.on("/fps", http_fps);
        server.on("/speed", http_speed);
        server.on("/brightness", http_brightness);
        server.on("/huespan", http_hue_span);
        server.on("/glitter/chance", http_glitter_chance);
        server.begin();
        Serial.println("HTTP server started");
}

void setup_eeprom() {
        EEPROM.begin(512);
        Serial.println("EEPROM initialized");

    //     EEPROM.put(BRIGHTNESS_ADDR, brightness);
    //     EEPROM.put(SPEED_ADDR, speed);
    //     EEPROM.put(FPS_ADDR, fps);
    //     EEPROM.put(HAIR_ADDR, gCurrentHair);
    //     EEPROM.put(EYE_ADDR, gCurrentEye);
    //     EEPROM.put(OVERLAY_ADDR, gCurrentOverlay);
    // #ifdef TAIL_LEDS
    //     EEPROM.put(TAIL_ADDR, gCurrentTail);
    // #endif
    //     EEPROM.commit();

        EEPROM.get(BRIGHTNESS_ADDR, brightness);
        EEPROM.get(SPEED_ADDR, speed);
        EEPROM.get(FPS_ADDR, fps);
        EEPROM.get(HAIR_ADDR, gCurrentHair);
        EEPROM.get(EYE_ADDR, gCurrentEye);
        EEPROM.get(OVERLAY_ADDR, gCurrentOverlay);
    #ifdef TAIL_LEDS
        EEPROM.get(TAIL_ADDR, gCurrentTail);
    #endif

        Serial.println("read defaults from EEPROM");
        Serial.println("brightness: " + String(brightness));
        Serial.println("fps: " + String(fps));
        Serial.println("speed: " + String(speed));
        Serial.println("gCurrentHair: " + String(gCurrentHair));
        Serial.println("gCurrentEye: " + String(gCurrentEye));
        Serial.println("gCurrentOverlay: " + String(gCurrentOverlay));
    #ifdef TAIL_LEDS
        Serial.println("gCurrentTail: " + String(gCurrentTail));
    #endif
}

/*
    hair pattern functions
 */
void fish_hair() {
        static uint8_t colour_index = 0;
        static uint8_t wave_index = 0;
        static uint8_t num_leds = sizeof(strip1) / sizeof(CRGB);
#ifdef STRIP5_LEDS
        static uint8_t last_strip_led = (sizeof(strip5) / sizeof(CRGB)) - 1;
#else
        static uint8_t last_strip_led = (sizeof(strip4) / sizeof(CRGB)) - 1;
#endif
        static uint8_t dir = 1;

        EVERY_N_MILLISECONDS(255 - speed) {

                // copy previous strip to make pattern move across the strips
#ifdef STRIP5_LEDS
                memcpy(strip5, strip4, sizeof(strip5));
#endif
                memcpy(strip4, strip3, sizeof(strip4));
                memcpy(strip3, strip2, sizeof(strip3));
                memcpy(strip2, strip1, sizeof(strip2));

                // then generate new pattern on first strip
                for (uint8_t led = 0; led < num_leds; led++) {
                        strip1[led] = ColorFromPalette(OceanColors_p, colour_index - (led * 2));
                }

                strip1[wave_index] = ColorFromPalette(OceanColors_p, 255 - colour_index);

                colour_index += 2;

                if (wave_index == last_strip_led) dir = -1;
                if (wave_index == 0) dir = 1;
                wave_index += dir;
        }
}

void rainbow_hair() {
        const uint8_t fade = 255 - brightness;

        fill_rainbow(strip1, STRIP1_LEDS, gHue, 255/sizeof(strip1));
        fill_rainbow(strip2, STRIP2_LEDS, gHue, 255/sizeof(strip2));
        fadeToBlackBy(strip2, STRIP2_LEDS, fade + 20);
        fill_rainbow(strip3, STRIP3_LEDS, gHue, 255/sizeof(strip3));
        fadeToBlackBy(strip3, STRIP3_LEDS, fade + 40);
        fill_rainbow(strip4, STRIP4_LEDS, gHue, 255/sizeof(strip4));
        fadeToBlackBy(strip4, STRIP4_LEDS, fade + 60);
#ifdef STRIP5_LEDS
        fill_rainbow(strip5, STRIP5_LEDS, gHue, 255/sizeof(strip5));
        fadeToBlackBy(strip5, STRIP5_LEDS, fade + 80);
#endif
}

void black_hair() {
        fill_solid(strip1, STRIP1_LEDS, CRGB::Black);
        fill_solid(strip2, STRIP2_LEDS, CRGB::Black);
        fill_solid(strip3, STRIP3_LEDS, CRGB::Black);
        fill_solid(strip4, STRIP4_LEDS, CRGB::Black);
#ifdef STRIP5_LEDS
        fill_solid(strip5, STRIP5_LEDS, CRGB::Black);
#endif
}

void white_hair() {
        fill_solid(strip1, STRIP1_LEDS, CRGB::White);
        fill_solid(strip2, STRIP2_LEDS, CRGB::White);
        fill_solid(strip3, STRIP3_LEDS, CRGB::White);
        fill_solid(strip4, STRIP4_LEDS, CRGB::White);
#ifdef STRIP5_LEDS
        fill_solid(strip5, STRIP5_LEDS, CRGB::White);
#endif
}

void test() {
        static uint8_t led = 0;
        static uint8_t colour = 0;

        switch(colour) {
        case 0: { rawleds[led] = CRGB::Red; break; }
        case 1: { rawleds[led] = CRGB::Green; break; }
        case 2: { rawleds[led] = CRGB::Blue; break; }
        }

        colour++;
        if (colour % 3 == 0) { led++; colour = 0; }
        if (led >= NUM_LEDS) { led = 0; }

        FastLED.delay(1000 - (1000 * speed/255));
}

/*
    eye pattern functions
 */

void fish_eyes() {
        static uint8_t i = 0;

        fill_palette(left_eye, 3, i, i+2, OceanColors_p, 255, LINEARBLEND);
        fill_palette(right_eye, 3, i, i+2, OceanColors_p, 255, LINEARBLEND);

        EVERY_N_MILLISECONDS(300) {
                i++;
        }
}

void white_eyes() {
        fill_solid(left_eye, 3, CRGB::White);
        fill_solid(right_eye, 3, CRGB::White);
}

void blue_eyes() {
        fill_solid(left_eye, 3, CRGB::Blue);
        fill_solid(right_eye, 3, CRGB::Blue);
}

void red_eyes() {
        fill_solid(left_eye, 3, CRGB::Red);
        fill_solid(right_eye, 3, CRGB::Red);
}

void yellow_eyes() {
        fill_solid(left_eye, 3, CRGB::Yellow);
        fill_solid(right_eye, 3, CRGB::Yellow);
}

void green_eyes() {
        fill_solid(left_eye, 3, CRGB::Green);
        fill_solid(right_eye, 3, CRGB::Green);
}

void black_eyes() {
        fill_solid(left_eye, 3, CRGB::Black);
        fill_solid(right_eye, 3, CRGB::Black);
}

/*
    tail patterns
 */
void fish_tail() {
#ifdef TAIL_LEDS
        static uint8_t index = 96;
        static uint8_t dir = 1;
        static uint8_t hue;

        for (uint8_t i = 0; i < (TAIL_LEDS / 2); i++) {
                if (index + (i * TAIL_LEDS) > 160)
                        hue = 160;

                tail[i] = CHSV(hue, 255, 255);
                tail[TAIL_LEDS-1-i] = CHSV(hue, 255, 255);
        }

        EVERY_N_MILLIS(100) {
                if (index > 160) dir = -1;
                if (index < 96) dir = 1;
                index += dir;
        }
#endif
}

void black_tail() {
#ifdef TAIL_LEDS
        fill_solid(tail, TAIL_LEDS, CRGB::Black);
#endif
}

/*
    overlay pattern functions
 */
void no_overlay() {
}

void glitter() {
        if (random8() < chance_of_glitter) {
                rawleds[ random16(NUM_LEDS) ] += CRGB::White;
        }
}

/*
    HTTP endpoint functions
 */
void http_on() {
        gCurrentHair = RAINBOW_HAIR;
        gCurrentOverlay = NO_OVERLAY;
        speed = 1;
        hue_span = NUM_LEDS;
        gHue = 0;
        server.send(200, "application/json", "{ \"state\": true }");
}

void http_off() {
        gCurrentHair = BLACK_HAIR;
        gCurrentOverlay = NO_OVERLAY;
        server.send(200, "application/json", "{ \"state\": false }");
}

void http_brightness() {
        // grab brightness from GET /brightness&value=<0-255>
        brightness = server.arg("value").toInt();

        if (brightness >= 0 || brightness <= 255) {
                FastLED.setBrightness(brightness);
                EEPROM.put(BRIGHTNESS_ADDR, brightness);
                EEPROM.commit();
                server.send(200, "application/json", "{ \"brightness\": " + String(brightness) + " }");
        }
}

void http_fps() {
        // grab fps from GET /fps&value=<1-255>
        fps = server.arg("value").toInt();
        if (fps == 0) { fps = 1; }
        EEPROM.put(FPS_ADDR, fps);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"fps\": " + String(fps) + " }");
}

void http_speed() {
        // grab speed from GET /speed&value=<1-255>
        speed = server.arg("value").toInt();
        if (speed == 0) { speed = 1; }
        EEPROM.put(SPEED_ADDR, speed);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"speed\": " + String(speed) + " }");
}

void http_hue_span() {
        // grab huespan from GET /huespan&value=<1-255>
        hue_span = server.arg("value").toInt();
        if (hue_span == 0) { hue_span = 1; }
        server.send(200, "application/json", "{ \"hue_pan\": " + String(hue_span) + " }");
}

void http_glitter_chance() {
        // grab value from GET /glitter/chance&value=<1-255>
        chance_of_glitter = server.arg("value").toInt();
        server.send(200, "application/json", "{ \"chance_of_glitter\": " + String(chance_of_glitter) + " }");
}

void http_hair() {
        gCurrentHair = server.arg("value").toInt();
        EEPROM.put(HAIR_ADDR, gCurrentHair);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"hair_pattern\": " + String(gCurrentHair) + " }");
}

void http_eyes() {
        gCurrentEye = server.arg("value").toInt();
        EEPROM.put(EYE_ADDR, gCurrentEye);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"eye_pattern\": " + String(gCurrentEye) + " }");
}

#ifdef TAIL_LEDS
void http_tail() {
        gCurrentTail = server.arg("value").toInt();
        EEPROM.put(TAIL_ADDR, gCurrentTail);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"tail_pattern\": " + String(gCurrentTail) + " }");
}
#endif

void http_hair_off() {
        gCurrentHair = BLACK_HAIR;
        EEPROM.put(HAIR_ADDR, gCurrentHair);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"hair_pattern\": \"off\" }");
}

void http_hair_rainbow() {
        gCurrentHair = RAINBOW_HAIR;
        EEPROM.put(HAIR_ADDR, gCurrentHair);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"hair_pattern\": \"rainbow\" }");
}

void http_hair_white() {
        gCurrentHair = WHITE_HAIR;
        EEPROM.put(HAIR_ADDR, gCurrentHair);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"hair_pattern\": \"white\" }");
}

void http_test() {
        gCurrentHair = TEST_HAIR;
        EEPROM.put(HAIR_ADDR, gCurrentHair);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"hair_pattern\": \"test\" }");
}

void http_overlay() {
        gCurrentOverlay = server.arg("value").toInt();
        EEPROM.put(OVERLAY_ADDR, gCurrentOverlay);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"overlay\": " + String(gCurrentOverlay) + " }");
}

void http_overlay_off() {
        gCurrentOverlay = NO_OVERLAY;
        EEPROM.put(OVERLAY_ADDR, gCurrentOverlay);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"overlay\": \"off\" }");
}

void http_overlay_glitter() {
        gCurrentOverlay = GLITTER_OVERLAY;
        EEPROM.put(OVERLAY_ADDR, gCurrentOverlay);
        EEPROM.commit();
        server.send(200, "application/json", "{ \"overlay\": \"glitter\" }");
}

/*
    helper functions
 */
void fill_rainbow_virtual(
        struct CRGB * p_first_led,
        int num_leds,
        const int *virtual_leds,
        uint8_t initial_hue,
        uint8_t delta_hue
        ) {
        CHSV hsv;
        hsv.hue = initial_hue;
        hsv.val = 255;
        hsv.sat = 240;
        for (int i = 0; i < num_leds; i++) {
                int virtual_led = virtual_leds[i];
                p_first_led[virtual_led] = hsv;
                hsv.hue += delta_hue;
        }
}
