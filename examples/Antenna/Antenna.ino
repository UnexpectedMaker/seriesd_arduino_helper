#include "WiFi.h"
#include <UMSeriesD.h>

UMSeriesD umseriesd;

bool external_antenna = false;

void setup()
{
    Serial.begin(115200);

    umseriesd.begin();
    umseriesd.setPixelBrightness(255 / 3);
    umseriesd.setPixelPower(true);

    // Set the RF Switch to onboard antenna - this is not needed as this happens in begin() above,
    // but shows how to set the state when required.
    umseriesd.setAntennaExternal(external_antenna);

    // Set WiFi to station mode and disconnect from an AP if it was previously connected.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup done");
}

void loop()
{
    Serial.printf("Scan start using the %s antenna!\n", (external_antenna ? "uFL" : "ONBOARD"));

    if (external_antenna)
        umseriesd.setPixelColor(UMSeriesD::color(0, 255, 0));
    else
        umseriesd.setPixelColor(UMSeriesD::color(0, 0, 255));

    // WiFi.scanNetworks will return the number of networks found.
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.printf("%2d", i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4ld", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2ld", WiFi.channel(i));
            Serial.print(" | ");
            switch (WiFi.encryptionType(i))
            {
            case WIFI_AUTH_OPEN:
                Serial.print("open");
                break;
            case WIFI_AUTH_WEP:
                Serial.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                Serial.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                Serial.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                Serial.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                Serial.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                Serial.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                Serial.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                Serial.print("WAPI");
                break;
            default:
                Serial.print("unknown");
            }
            Serial.println();
            delay(10);
        }
    }
    Serial.println("");

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    // Wait a bit before scanning again.
    delay(1000);

    // Toggle the bool and set the antenna output.
    external_antenna = !external_antenna;
    umseriesd.setAntennaExternal(external_antenna);
}
