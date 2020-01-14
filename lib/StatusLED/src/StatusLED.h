/* StatusLED.h    */

#include "FastLED.h"

// FastLED
#define DATA_PIN 18
#define NUM_LEDS 1
// #define LED_DRIVER NEOPIXEL

CRGB leds[NUM_LEDS];

class StatusLED
{
public:
    StatusLED();
    void drivers(bool pinned_LED);
    void on(CRGB colour);
    void off();
    void flash(CRGB colour, int duration_millis);
};

StatusLED::StatusLED()
{
}

void StatusLED::drivers(bool pinned_LED)
// NOTE: the breadboard needs the NEOPIXEL driver, the pinned LED needs the WS2811.
// TODO: Make driver a preference
{
    if (pinned_LED)
        FastLED.addLeds<WS2811, DATA_PIN>(leds, NUM_LEDS);
    else
        FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void StatusLED::on(CRGB colour)
{
    leds[0] = colour;
    FastLED.show();
}

void StatusLED::off()
{
    leds[0] = CRGB::Black;
    FastLED.show();
}

void StatusLED::flash(CRGB colour, int duration_millis)
{
    on(colour);
    delay(duration_millis);
    off();
    delay(duration_millis);
}