#include "FastLED.h"

#define RING 16
#define NUM_RINGS 5
#define NUM_LEDS (RING*NUM_RINGS)
#define DATA_PIN 4

#define LOW_BRIGHTNESS 80
#define MAX_BRIGHTNESS 255

#define NOISE_SCALE 30

#define NIGHTTIME 1

CRGBArray<NUM_LEDS> leds;

DEFINE_GRADIENT_PALETTE(twilightPalette) {
      0,   0, 171,  85, // aqua
     85,   0,   0, 255, // blue
    170,  85,   0, 171, // purple
    255, 171,   0,  85, // pink
};

DEFINE_GRADIENT_PALETTE(sunrisePalette) {
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
    if (isNighttime()) {
        sunset();
    } else {
        sunrise();
    }
}

bool isNighttime() {
    return NIGHTTIME;
}

void sunrise() {
    static uint8_t active = 0; // active ring
    static uint8_t stage = 0;

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
            paletteBlend(0, active, 10, true, LOW_BRIGHTNESS);
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
        case 2: // graduate to deep sunrise colors, yellow through red
            static uint8_t delay = 5;
            targetPalette = sunrisePalette;
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
                        uint8_t index = inoise8(led*NOISE_SCALE, dist+led*NOISE_SCALE);
                        leds[led] = ColorFromPalette(oldPalette, index, LOW_BRIGHTNESS);
                    }
                }
            }
            dist += beatsin8(10, 1, 4);
            break;
        default: // fade to bright white
            targetPalette = CRGBPalette16(CHSV(60, 0, 255));
            paletteBlend(beatsin8(10, 1, 4), -1, 40, false, MAX_BRIGHTNESS);
    }
    FastLED.show();
    FastLED.delay(5);
}

void sunset() {
    static uint8_t stage = 0;
    static uint8_t streak = 0;
    static uint8_t twinkleCount = 0;
    static uint8_t delay = 5;
    switch(stage) {
        case 0:
            meteorRain(RING, RING*3, 64);
            streak++;
            if (streak == 3) stage++;
            break;
        case 1:
            EVERY_N_MILLIS(100) {
                twinkle(twinkleCount++);
            }
            if (twinkleCount == NUM_LEDS) {
                stage++;
                currentPalette = CRGBPalette16(CRGB(148, 0, 148));
            }
            break;
        case 2:
            targetPalette = twilightPalette;
            if (currentPalette == targetPalette) {
                paletteBlend(0, -1, 40, true, LOW_BRIGHTNESS);
                EVERY_N_SECONDS(2) {
                    if (delay > 0) {
                        delay--;
                    } else {
                        stage++;
                    }
                }
            } else {
                paletteBlend(255, -1, 40, true, LOW_BRIGHTNESS);
            }
            break;
        default:
            targetPalette = CRGBPalette16(CRGB::White);
            if (currentPalette != targetPalette) {
                paletteBlend(0, -1, 40, true, LOW_BRIGHTNESS);
            }
    }
}

// https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectMeteorRain
void meteorRain(byte meteorSize, byte meteorTrailDecay, int SpeedDelay) {
    CHSV hsv(212, 128, 255); // electric purple
    for(int i = 0; i < 16*NUM_LEDS; i+=RING) {
        // fade brightness all LEDs one step
        for(int j=0; j<NUM_LEDS; j++) {
            meteorTrailDecay -= j;
            if (meteorTrailDecay < 0) meteorTrailDecay = 0;
            if ((random(j)<meteorSize)) {
                leds[j].fadeToBlackBy(meteorTrailDecay);
            }
        }

        // draw meteor
        for(int j = 0; j < meteorSize; j++) {
            if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
                leds[i-j] = hsv;
            }
        }

        FastLED.show();
        FastLED.delay(SpeedDelay);
    }
}

// https://gist.github.com/kriegsman/88954aae22b03a664081
void twinkle(uint8_t chance) {
    enum { SteadyDim, GettingBrighter, GettingDimmerAgain };
    static uint8_t PixelState[NUM_LEDS];
    const CRGB base(32, 0, 32);
    const CRGB peak(128, 0, 128);
    const CRGB delta(4, 0, 4);
    for( uint16_t i = 0; i < NUM_LEDS; i++) {
        if( PixelState[i] == SteadyDim) {
            // this pixels is currently: SteadyDim
            // so we randomly consider making it start getting brighter
            if( random8() < chance) {
                PixelState[i] = GettingBrighter;
            }
        } else if( PixelState[i] == GettingBrighter ) {
            // this pixels is currently: GettingBrighter
            // so if it's at peak color, switch it to getting dimmer again
            if( leds[i] >= peak ) {
                PixelState[i] = GettingDimmerAgain;
            } else {
                // otherwise, just keep brightening it:
                leds[i] += delta;
            }
        } else { // getting dimmer again
            // this pixels is currently: GettingDimmerAgain
            // so if it's back to base color, switch it to steady dim
            if( leds[i] <= base ) {
                leds[i] = base; // reset to exact base color, in case we overshot
                PixelState[i] = SteadyDim;
            } else {
                // otherwise, just keep dimming it down:
                leds[i] -= delta;
            }
        }
    }
    FastLED.show();
    FastLED.delay(20);
}

void paletteBlend(fract8 blendWithPrevious, int activeRing, uint8_t delay, bool noisy, uint8_t brightness) {
    static uint16_t dist;
    EVERY_N_MILLIS(delay) {
        nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
    }
    CRGBPalette16 palette;
    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t index;
        if (noisy) {
            index = inoise8(i*NOISE_SCALE, dist+i*NOISE_SCALE);
        } else {
            index = float(255*i)/NUM_LEDS;
        }
        CRGB color;
        if (activeRing < 0 || (i >= activeRing*RING && i < (activeRing+1)*RING)) {
            color = ColorFromPalette(currentPalette, index, brightness);
        } else if (i < activeRing*RING) {
            color = ColorFromPalette(targetPalette, index, brightness);
        } else {
            color = ColorFromPalette(oldPalette, index, brightness);
        }
        if (blendWithPrevious) {
            leds[i] = blend(leds[i], color, blendWithPrevious);
        } else {
            leds[i] = color;
        }
    }
    dist += beatsin8(10, 1, 4);
    FastLED.show();
    FastLED.delay(5);
}
