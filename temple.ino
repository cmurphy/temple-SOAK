#include "FastLED.h"

#define NUM_LEDS 80
#define DATA_PIN 4
#define RING 16
#define NUM_RINGS 5
CRGBArray<NUM_LEDS> leds;

DEFINE_GRADIENT_PALETTE(twilightPalette) {
      0,   0, 171,  85, // aqua
     85,   0,   0, 255, // blue
    170,  85,   0, 171, // purple
    255, 171,   0,  85, // pink
};

DEFINE_GRADIENT_PALETTE(sunsetPaletteDef) {
      0, 255,   0,  0, // red
    128, 171,  85,  0, // orange
    255, 171, 171,  0, // yellow
};

static CRGBPalette16 currentPalette(CHSV(160, 255, 255));
static CRGBPalette16 targetPalette(twilightPalette);
CRGBPalette16 sunsetPalette(sunsetPaletteDef);

void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(125);
}

void loop() {
    sunset();
    FastLED.show();
    FastLED.delay(10);
}

void sunset() {
    static uint8_t hue = 160; // deep blue
    static uint8_t brightness = 0;
    const uint8_t maxBrightness = 250;
    static uint8_t active = 0; // active ring
    static uint8_t stage = 0;
    CHSV hsv;
    hsv.sat = 255;
    switch (stage) {
        case 0: // fill rings one by one with deep blue
            hsv.hue = hue;
            for (int i = 0; i < NUM_RINGS; i++) {
                for (int j = 0; j < RING; j++) {
                    int led = i*RING+j;
                    if (i == active) {
                        hsv.val = brightness;
                        leds[led] = hsv;
                    } else if (i < active) {
                        hsv.val = maxBrightness;
                        leds[led] = hsv;
                    } else {
                        leds[led] = CRGB::Black;
                    }
                }
            }
            if (brightness >= maxBrightness) {
                brightness = 0;
                if (active == NUM_RINGS) {
                    stage++;
                    active = 0;
                } else {
                    active++;
                }
            } else {
                brightness += 2;
            }
            break;
        case 1: // map to twilight colors, aqua through pink
            // https://pastebin.com/r70Qk6Bn
            if (currentPalette == targetPalette) {
                stage++;
                break;
            }
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
            }
            #define scale 30
            static uint16_t dist;
            for (int i = 0; i < NUM_LEDS; i++) {
                uint8_t index = inoise8(i*scale, dist+i*scale);
                leds[i] = ColorFromPalette(currentPalette, index, 255);
            }
            dist += beatsin8(10, 1, 4);
            break;
        case 2: // graduate to deep sunset colors, yellow through red
            if (currentPalette == sunsetPalette) {
                stage++;
                break;
            }
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, sunsetPalette, 12);
            }
            for (int i = 0; i < NUM_LEDS; i++) {
                uint8_t index = float(255*i)/NUM_LEDS;
                leds[i] = ColorFromPalette(currentPalette, index);
            }
            break;
        default:
            CRGBPalette16 finalPalette(CHSV(60, 0, 255));
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, finalPalette, 24);
            }
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = ColorFromPalette(currentPalette, float(255*i)/NUM_LEDS);
            }
    }
}
