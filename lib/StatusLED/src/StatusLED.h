/* StatusLED.h    */

#include "FastLED.h"

CRGB leds[NUM_LEDS];

class StatusLED
{
public:
    StatusLED();
    void drivers(bool pinned_LED);
    void on(CRGB colour);
    void off();
    void flash(CRGB colour, int duration_millis);
    void colour(float value, float min, float max);
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

// 'breathing' function https://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
float breathe()
{
    return (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
}

void StatusLED::colour(float val, float min, float max)
{
    const int hue_max = 160; // blue in the FastLED 'rainbow' colour map
    CHSV pixelColour;

    val = constrain(val, min, max);

    pixelColour.sat = 255;
    pixelColour.hue = map(val, min, max, hue_max, 0);
    pixelColour.val = map(breathe(), 0, 255, 70, 255);
    // pixelColour.val = 128;

    leds[0] = pixelColour;
    FastLED.show();
}
