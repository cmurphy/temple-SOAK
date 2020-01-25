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
            static CRGBPalette16 currentPalette(CHSV(160, 255, 255));
            static CRGBPalette16 targetPalette(twilightPalette);
            if (currentPalette == targetPalette) {
                stage++;
            }
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
            }
            #define scale 30
            static uint16_t dist;
            for (int i = 0; i < NUM_LEDS; i++) {
                uint8_t index = inoise8(i*scale, dist+i*scale);
                leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);
            }
            dist += beatsin8(10, 1, 4);
            break;
        default:
            hsv.hue = 160;
            hsv.val = 255;
            hsv.sat = 255;
            CHSV target = CHSV(60, 0, 255);
            CHSV current = hsv;
            static uint8_t amount = 0;
            if (amount < 255) {
                current = blend(current, target, amount++);
                leds.fill_solid(current);
            }
    }
}
