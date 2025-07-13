#include <UMSeriesD.h>

UMSeriesD umseriesd;

void setup()
{
    Serial.begin(115200);

    // Delay to allow native USB to kick in to get serial output
    delay(2000);

    // Initialize all board peripherals, call this first
    umseriesd.begin();
}

void loop()
{
    // Light sensor voltage goes up to about 3.3v
    float light = umseriesd.getLightSensorVoltage();

    // View this with the arduino serial plotter (in the tools menu)
    Serial.println(light);

    delay(50);
}