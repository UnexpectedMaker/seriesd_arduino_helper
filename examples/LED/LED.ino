#include <UMSeriesD.h>

UMSeriesD umseriesd;

void setup()
{
    // Initialize all board peripherals, call this first
    umseriesd.begin();

    // Brightness is 0-255. We set it to 1/3 brightness here
    umseriesd.setPixelBrightness(255 / 3);

    // Enable the power to the RGB LED.
    // Off by default so it doesn't use current when the LED is not required.
    umseriesd.setPixelPower(true);
}

int color = 0;

void loop()
{
    // colorWheel cycles red, orange, ..., back to red at 256
    umseriesd.setPixelColor(UMSeriesD::colorWheel(color));
    color++;

    // On the FeatherS3D, toggle the LED twice per cycle
#ifdef ARDUINO_FEATHERS3
    if (color % 128 == 0)
    {
        umseriesd.toggleBlueLED();
    }
#endif

    delay(15);
}