#include "fish.h"
#include "password.h"

#ifdef RACHEL
#define HOSTNAME   "rachel.fish"
#define NUM_LEDS    130
#define STRIP1_LEDS 29
#define STRIP2_LEDS 25
#define STRIP3_LEDS 23
#define STRIP4_LEDS 18
#define STRIP5_LEDS 13
#define TAIL_LEDS   16
#elif defined(ROSIE)
#define HOSTNAME   "rosie.fish"
#define NUM_LEDS    116
#define STRIP1_LEDS 27
#define STRIP2_LEDS 34
#define STRIP3_LEDS 23
#define STRIP4_LEDS 15
#define STRIP5_LEDS 11
#elif defined(RONIN)
#define HOSTNAME   "ronin.fish"
#define NUM_LEDS    120
#define STRIP1_LEDS 30
#define STRIP2_LEDS 26
#define STRIP3_LEDS 26
#define STRIP4_LEDS 18
#define STRIP5_LEDS 14
#elif defined(GERTRUDE)
#define HOSTNAME   "gertrude.fish"
#define NUM_LEDS    82
#define STRIP1_LEDS 25
#define STRIP2_LEDS 22
#define STRIP3_LEDS 17
#define STRIP4_LEDS 12
#elif defined(GREG)
#define HOSTNAME   "greg.fish"
#define NUM_LEDS    11
#define STRIP1_LEDS 1
#define STRIP2_LEDS 1
#define STRIP3_LEDS 1
#define STRIP4_LEDS 1
#define STRIP5_LEDS 1
#elif defined(GORDON)
#define HOSTNAME   "gordon.fish"
#define NUM_LEDS    108
#define STRIP1_LEDS 29
#define STRIP2_LEDS 28
#define STRIP3_LEDS 25
#define STRIP4_LEDS 20
#elif defined(YAMES)
#define HOSTNAME   "yames.fish"
#define NUM_LEDS    97
#define STRIP1_LEDS 29
#define STRIP2_LEDS 24
#define STRIP3_LEDS 22
#define STRIP4_LEDS 16
#elif defined(YASMIN)
#define HOSTNAME   "yasmin.fish"
#define NUM_LEDS    104
#define STRIP1_LEDS 30
#define STRIP2_LEDS 26
#define STRIP3_LEDS 24
#define STRIP4_LEDS 18
#elif defined(YOLANDA)
#define HOSTNAME   "yolanda.fish"
#define NUM_LEDS    92
#define STRIP1_LEDS 31
#define STRIP2_LEDS 25
#define STRIP3_LEDS 17
#define STRIP4_LEDS 13
#endif

#ifdef ESP8266
#define DATA_PIN  D1
#define CHIPSET   NEOPIXEL
#else
#define DATA_PIN  MOSI
#define CLOCK_PIN SCK
#define CHIPSET   SK9822
#endif

#define COLOR_ORDER BGR

const char *ssid = HOSTNAME;
const char *password = WIFI_PASSWORD;
const char *hostname = HOSTNAME;

// global brightness, fps, speed, hueSpan, hue and the currentLed
unsigned int brightness = 255;                        // 0 - 255
unsigned int fps = 80;                // 1 - 255
unsigned int speed = 200;             // 1 - 255
unsigned int hue_span = NUM_LEDS;
unsigned int chance_of_glitter = 60;

#define DNS_PORT 53

#define FISH_HAIR    0
#define RAINBOW_HAIR 1
#define WHITE_HAIR   2
#define TEST_HAIR    3
#define BLACK_HAIR     4

#define FISH_EYE   0
#define WHITE_EYE  1
#define BLUE_EYE   2
#define RED_EYE    3
#define YELLOW_EYE 4
#define GREEN_EYE  5
#define BLACK_EYE  6

#define FISH_TAIL  0
#define BLACK_TAIL 1

#define NO_OVERLAY      0
#define GLITTER_OVERLAY 1

// init functions
void setup_wifi();
void setup_dns();
void setup_server();
void setup_led();

// pattern functions
void test();
void fish_hair();
void rainbow_hair();
void black_hair();
void white_hair();

void fish_eyes();
void white_eyes();
void blue_eyes();
void red_eyes();
void yellow_eyes();
void green_eyes();
void black_eyes();

void fish_tail();
void black_tail();

// overlay pattern functions
void no_overlay();
void glitter();

// HTTP endpoint functions
void http_on();
void http_off();

void http_brightness();
void http_fps();
void http_speed();
void http_hue_span();
void http_glitter_chance();

void http_test();
void http_hair();
void http_eyes();
void http_tail();
void http_hair_white();
void http_hair_rainbow();
void http_hair_off();

void http_overlay();
void http_overlay_glitter();
void http_overlay_off();

void fill_rainbow_virtual(
        struct CRGB * p_first_led,
        int num_leds,
        const int *virtual_leds,
        uint8_t initial_hue,
        uint8_t delta_hue
        );

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*simple_function_list[])();
simple_function_list gHairPatterns = { fish_hair, rainbow_hair, white_hair, test, black_hair };
simple_function_list gEyePatterns = { fish_eyes, white_eyes, blue_eyes, red_eyes, yellow_eyes, green_eyes, black_eyes };
simple_function_list gTailPatterns = { fish_tail, black_tail };
simple_function_list gOverlays = { no_overlay, glitter };
