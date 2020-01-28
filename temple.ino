#include "FastLED.h"

#define RING 16
#define NUM_RINGS 5
#define NUM_LEDS (RING*NUM_RINGS)
#define DATA_PIN 4

#define LOW_BRIGHTNESS 80
#define MAX_BRIGHTNESS 255

CRGBArray<NUM_LEDS> leds;

DEFINE_GRADIENT_PALETTE(twilightPalette) {
      0,   0, 171,  85, // aqua
     85,   0,   0, 255, // blue
    170,  85,   0, 171, // purple
    255, 171,   0,  85, // pink
};

DEFINE_GRADIENT_PALETTE(sunsetPalette) {
      0, 255,   0,  0, // red
    128, 171,  85,  0, // orange
    255, 171, 171,  0, // yellow
};

static CRGBPalette16 currentPalette(CRGB::Black);
static CRGBPalette16 oldPalette(currentPalette);
static CRGBPalette16 targetPalette(CRGB::DeepSkyBlue);

void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    leds.fill_solid(CRGB::Black);
}

void loop() {
    sunset();
    FastLED.show();
    FastLED.delay(5);
}

void sunset() {
    static uint8_t active = 0; // active ring
    static uint8_t stage = 0;

    const  uint8_t  scale = 30;
    static uint16_t dist;

    switch (stage) {
        case 0: // fill rings one by one with deep blue
            EVERY_N_MILLIS(1) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 255);
            }
            leds(active*RING, (active+1)*RING-1).fill_solid(ColorFromPalette(currentPalette, 0, LOW_BRIGHTNESS));
            if (active > 0) leds(0, active*RING-1).fill_solid(ColorFromPalette(targetPalette, 0, LOW_BRIGHTNESS));
            if (currentPalette == targetPalette) {
                if (active == NUM_RINGS - 1) {
                    stage++;
                    active = 0;
                    oldPalette = currentPalette;
                } else {
                    currentPalette = oldPalette;
                    active++;
                }
            }
            break;
        case 1: // map to twilight colors, aqua through pink
            // https://pastebin.com/r70Qk6Bn
            targetPalette = twilightPalette;
            EVERY_N_MILLIS(10) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
            }
            for (int i = 0; i < NUM_RINGS; i++) {
                for (int j = 0; j < RING; j++) {
                    int led = i*RING+j;
                    uint8_t index = inoise8(led*scale, dist+led*scale);
                    if (i == active) {
                        leds[led] = ColorFromPalette(currentPalette, index, LOW_BRIGHTNESS);
                    } else if (i < active) {
                        leds[led] = ColorFromPalette(targetPalette, index, LOW_BRIGHTNESS);
                    } else {
                        leds[led] = ColorFromPalette(oldPalette, index, LOW_BRIGHTNESS);
                    }
                }
            }
            dist += beatsin8(10, 1, 4);
            if (currentPalette == targetPalette) {
                if (active == NUM_RINGS) {
                    stage++;
                    active = 0;
                    oldPalette = currentPalette;
                } else {
                    currentPalette = oldPalette;
                    active++;
                }
            }
            break;
        case 2: // graduate to deep sunset colors, yellow through red
            static uint8_t delay = 5;
            targetPalette = sunsetPalette;
            if (currentPalette == targetPalette) {
                stage++;
                break;
            }
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 12);
            }
            EVERY_N_SECONDS(2) {
                if (delay > 0) {
                    delay--;
                } else {
                    active++;
                }
            }
            for (int i = 0; i < NUM_RINGS; i++) {
                for (int j = 0; j < RING; j++) {
                    int led = i*RING+j;
                    if (i <= active) {
                        uint8_t index = float(255*led)/NUM_LEDS;
                        CRGB color = ColorFromPalette(currentPalette, index, MAX_BRIGHTNESS);
                        leds[led] = blend(leds[led], color, beatsin8(10, 1, 4));
                    } else {
                        uint8_t index = inoise8(led*scale, dist+led*scale);
                        leds[led] = ColorFromPalette(oldPalette, index, LOW_BRIGHTNESS);
                    }
                }
            }
            dist += beatsin8(10, 1, 4);
            break;
        default: // fade to bright white
            targetPalette = CRGBPalette16(CHSV(60, 0, 255));
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
            }
            for (int i = 0; i < NUM_LEDS; i++) {
                CRGB color = ColorFromPalette(currentPalette, float(255*i)/NUM_LEDS, MAX_BRIGHTNESS);
                leds[i] = blend(leds[i], color, beatsin8(10, 1, 4));
            }
    }
}
