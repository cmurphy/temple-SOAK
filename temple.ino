#include "FastLED.h"

#define RING 16
#define NUM_RINGS 5
#define NUM_LEDS (RING*NUM_RINGS)
#define DATA_PIN 4

#define LOW_BRIGHTNESS 80
#define MAX_BRIGHTNESS 255

#define NOISE_SCALE 30
#define ACTION_TIMEOUT 5

#define NIGHTTIME 0

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
    170, 128, 128,  0, // yellow
    255, 64,    0,  0, // pink
};

CRGBPalette16 currentPalette(CRGB::White);
CRGBPalette16 oldPalette(currentPalette);
CRGBPalette16 targetPalette(CRGB::DeepSkyBlue);

enum { ActionA = '1', ActionB, ActionC };

void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    if (isNighttime()) {
        leds.fill_solid(CRGB::Black);
        currentPalette = CRGBPalette16(CRGB::Black);
    } else {
        CRGB dimWhite(LOW_BRIGHTNESS, LOW_BRIGHTNESS, LOW_BRIGHTNESS);
        leds.fill_solid(dimWhite);
        currentPalette = CRGBPalette16(CRGB::White);
    }
    oldPalette = currentPalette;
    // TODO: synchronize time with RTC

    // Setup serial input
    // TODO: change to setup button on pin
    Serial.begin(9600);
}

void loop() {
    static char next = 0;
    if (Serial.available()) {
        next = processButtonPress();
    }
    switch(next) {
        case 0:
            break;
        case ActionA:
            next = actionA();
            return;
        case ActionB:
            next = actionB();
            return;
        case ActionC:
            next = actionC();
            return;
        default:
            Serial.printf("Invalid input '%c'", next);
    }
    if (isNighttime()) {
        sunset();
    } else {
        sunrise();
    }
}

bool isNighttime() {
    // TODO: use RTC to check time
    return NIGHTTIME;
}

void sunrise() {
    static uint8_t active = 0; // active ring
    static uint8_t stage = 0;

    static uint16_t dist;

    switch (stage) {
        case 0: // fill rings one by one with deep blue
            targetPalette = CRGBPalette16(CRGB::DeepSkyBlue);
            EVERY_N_MILLIS(1) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 128);
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
            paletteBlend(0, active, 1, 80, true, LOW_BRIGHTNESS);
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
            if (currentPalette == targetPalette && active >= NUM_RINGS) {
                stage++;
                break;
            }
            EVERY_N_MILLIS(40) {
                nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
            }
            EVERY_N_SECONDS(1) {
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
                        leds[led] = blend(leds[led], color, beatsin8(30, 1, 8));
                    } else {
                        uint8_t index = inoise8(led*NOISE_SCALE, dist+led*NOISE_SCALE);
                        leds[led] = ColorFromPalette(oldPalette, index, LOW_BRIGHTNESS);
                    }
                }
            }
            dist += beatsin8(10, 1, 4);
            break;
        case 3: // lighten to white
            targetPalette = CRGBPalette16(CRGB::White);
            paletteBlend(128, -1, 20, 12, false, MAX_BRIGHTNESS);
            if (currentPalette == targetPalette) {
                stage++;
            }
            break;
        default: // fade to black
            leds.fadeToBlackBy(1);
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
                paletteBlend(0, -1, 40, 24, true, LOW_BRIGHTNESS);
                EVERY_N_SECONDS(2) {
                    if (delay > 0) {
                        delay--;
                    } else {
                        stage++;
                    }
                }
            } else {
                paletteBlend(255, -1, 40, 24, true, LOW_BRIGHTNESS);
            }
            break;
        default:
            targetPalette = CRGBPalette16(CRGB::White);
            if (currentPalette != targetPalette) {
                paletteBlend(0, -1, 40, 24, true, LOW_BRIGHTNESS);
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

void paletteBlend(fract8 blendWithPrevious, int activeRing, uint8_t delay, uint8_t rate, bool noisy, uint8_t brightness) {
    static uint16_t dist;
    EVERY_N_MILLIS(delay) {
        nblendPaletteTowardPalette(currentPalette, targetPalette, rate);
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

char processButtonPress() {
    int bytesAvailable = Serial.available();
    if (!bytesAvailable) {
        return 0;
    }
    if (bytesAvailable > 2) {
        Serial.println("Invalid input");
    }
    char byte = Serial.read();
    Serial.read(); // clear newline
    return byte;
}

// Confetti effect
// https://github.com/atuline/FastLED-Demos/blob/master/confetti/confetti.ino
// TODO: consider how this looks when rings are in translucent containers
char actionA() {
    static char    timeout = ACTION_TIMEOUT;
    static uint8_t thisfade = 8;
    static int     thishue = 50;
    static uint8_t thisinc = 1;
    static uint8_t thissat = 100;
    static uint8_t thisbri = 255;
    static int     huediff = 256;
    EVERY_N_SECONDS(1) {
        timeout--;
    }
    uint8_t secondHand = (millis() / 1000) % 15;
    static uint8_t lastSecond = 99;
    if (lastSecond != secondHand) {
        lastSecond = secondHand;
        switch(secondHand) {
            case  0: thisinc=1; thishue=192; thissat=255; thisfade=2; huediff=256; break;
            case  5: thisinc=2; thishue=128; thisfade=8; huediff=64; break;
            case 10: thisinc=1; thishue=random16(255); thisfade=1; huediff=16; break;
            case 15: break;
        }
    }

    EVERY_N_MILLISECONDS(5) {
        fadeToBlackBy(leds, NUM_LEDS, thisfade);
        int pos = random16(NUM_LEDS);
        leds[pos] += CHSV((thishue + random16(huediff))/4 , thissat, thisbri);
        thishue = thishue + thisinc;
    }
    FastLED.show();
    if (!timeout) {
        timeout = ACTION_TIMEOUT;
        return 0;
    }
    return ActionA;
}

// Fire effect
// https://github.com/FastLED/FastLED/blob/dcbf39933f51a2a0e4dfa0a2b3af4f50040df5c9/examples/Fire2012/Fire2012.ino
char actionB() {
    static char    timeout = ACTION_TIMEOUT;
    EVERY_N_SECONDS(1) {
        timeout--;
    }

    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];
    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100
    #define COOLING  55

    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    #define SPARKING 120
    // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
        int y = random8(7);
        heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
        leds[j] = HeatColor( heat[j]);
    }

    FastLED.show();
    FastLED.delay(16);
    if (!timeout) {
        timeout = ACTION_TIMEOUT;
        return 0;
    }
    return ActionB;
}

char actionC() {Serial.println("action c");}
