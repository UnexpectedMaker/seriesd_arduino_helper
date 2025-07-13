/*
 * Unexpected Maker Series[D] Arduino Helper Library
 * Release version 1.0.0
 * Requires ESP32 Arduino Core 3.x - 3.2.1 for EdgeS3[D] support
 */

#ifndef _UMSeriesD_H
#define _UMSeriesD_H

#include <Arduino.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
#include <Wire.h>

#if defined(ARDUINO_FEATHERS3)
#define ALS_ADC_CHANNEL ADC1_GPIO4_CHANNEL
#define ALS_ADC_PIN 4
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define RF_SWITCH 41
#elif defined(ARDUINO_TINYS3)
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define RF_SWITCH 38
#elif defined(ARDUINO_PROS3)
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define RF_SWITCH 11
#elif defined(ARDUINO_EDGES3D)
#define RF_SWITCH 38
#else
#error \
    "The board you have selected is not compatible with the UMS3 helper library"
#endif

class UMSeriesD
{
    enum class MAX17048_REG
    {
        VCELL = 0x02,
        SOC = 0x04,
        MODE = 0x06,
        VERSION = 0x08,
        HIBRT = 0x0A,
        CONFIG = 0x0C,
        VALRT = 0x14,
        CRATE = 0x16,
        VRESET_ID = 0x18,
        STATUS = 0x1A,
        TABLE = 0x40,
        CMD = 0xFE
    };

    const uint8_t I2C_ADDR = 0x36;

public:
#if defined(HAS_RGB)
    UMSeriesD() : brightness(255) {}
#else
    UMSeriesD() {}
#endif

    void begin()
    {
#if defined(HAS_RGB)
        // RGB_PWR is LDO2 on boards that have it
        pinMode(RGB_PWR, OUTPUT);
        rmtInit(RGB_DATA, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
#endif

#if defined(ARDUINO_FEATHERS3)
        pinMode(LED_BUILTIN, OUTPUT);
#endif

#if defined(ARDUINO_FEATHERS3)
        analogSetPinAttenuation(ALS_ADC_PIN, ADC_11db);
#endif

#if defined(HAS_VBUS_SENSE)
        pinMode(VBUS_SENSE, INPUT);
#endif

        // Setup the RF Switch IO and set to onboard
        pinMode(RF_SWITCH, OUTPUT);
        digitalWrite(RF_SWITCH, false);
    }

    void setLDO2Power(bool on)
    {
#if defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3)
        digitalWrite(LDO2, on);
#else
        Serial.println("UMSeriesD ERROR: setLDO2Power not available");
#endif
    }

    void setPixelPower(bool on)
    {
#if defined(HAS_RGB)
        digitalWrite(RGB_PWR, on);
#else
        Serial.println("UMSeriesD ERROR: setPixelPower not available");
#endif
    }

    void setPixelColor(uint8_t r, uint8_t g, uint8_t b)
    {
#if defined(HAS_RGB)
        pixel_color[0] = g;
        pixel_color[1] = r;
        pixel_color[2] = b;
        writePixel();
#else
        Serial.println("UMSeriesD ERROR: setPixelColor not available");
#endif
    }

    void setPixelColor(uint32_t rgb)
    {
#if defined(HAS_RGB)
        setPixelColor(rgb >> 16, rgb >> 8, rgb);
#else
        Serial.println("UMSeriesD ERROR: setPixelColor not available");
#endif
    }

    void setPixelBrightness(uint8_t brightness)
    {
#if defined(HAS_RGB)
        this->brightness = brightness;
        writePixel();
#else
        Serial.println("UMSeriesD ERROR: setPixelBrightness not available");
#endif
    }

    void writePixel()
    {
#if defined(HAS_RGB)
        setPixelPower(true);
        while (micros() - next_rmt_write < 350)
        {
            yield();
        }
        int index = 0;
        for (auto chan : pixel_color)
        {
            uint8_t value = chan * (brightness + 1) >> 8;
            for (int bit = 7; bit >= 0; bit--)
            {
                if ((value >> bit) & 1)
                {
                    rmt_data[index].level0 = 1;
                    rmt_data[index].duration0 = 8;
                    rmt_data[index].level1 = 0;
                    rmt_data[index].duration1 = 4;
                }
                else
                {
                    rmt_data[index].level0 = 1;
                    rmt_data[index].duration0 = 4;
                    rmt_data[index].level1 = 0;
                    rmt_data[index].duration1 = 8;
                }
                index++;
            }
        }

        rmtWrite(RGB_DATA, rmt_data, 3 * 8, RMT_WAIT_FOR_EVER);
        next_rmt_write = micros();
#else
        Serial.println("UMSeriesD ERROR: writePixel not available");
#endif
    }

    static uint32_t color(uint8_t r, uint8_t g, uint8_t b)
    {
        return (r << 16) | (g << 8) | b;
    }

    static uint32_t colorWheel(uint8_t pos)
    {
        if (pos < 85)
        {
            return color(255 - pos * 3, pos * 3, 0);
        }
        else if (pos < 170)
        {
            pos -= 85;
            return color(0, 255 - pos * 3, pos * 3);
        }
        else
        {
            pos -= 170;
            return color(pos * 3, 0, 255 - pos * 3);
        }
    }

    void setBlueLED(bool on)
    {
#if defined(ARDUINO_FEATHERS3)
        digitalWrite(LED_BUILTIN, on);
#else
        Serial.println("UMSeriesD ERROR: setBlueLED not available");
#endif
    }

    void toggleBlueLED()
    {
#if defined(ARDUINO_FEATHERS3)
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
#else
        Serial.println("UMSeriesD ERROR: toggleBlueLED not available");
#endif
    }

    float getLightSensorVoltage()
    {
#if defined(ARDUINO_FEATHERS3)
        uint32_t millivolts = analogReadMilliVolts(ALS_ADC_PIN);
        return (float)(millivolts);
#else
        Serial.println("UMSeriesD ERROR: getLightSensorVoltage not available");
        return 0.0f;
#endif
    }

    void FG_setup(TwoWire &w) { wire = &w; }

    float getBatteryVoltage()
    {
        return ((float)i2c_read(MAX17048_REG::VCELL) * 78.125f / 1000000.f);
    }

    uint8_t FG_version()
    {
        return (uint8_t)i2c_read(MAX17048_REG::VERSION);
    }

    void setAntennaExternal(bool state)
    {
        // Set the RF Switch HIGH for External and LOW for Internal
        digitalWrite(RF_SWITCH, state);
    }

    bool getVbusPresent()
    {
#if defined(HAS_VBUS_SENSE)
        return digitalRead(VBUS_SENSE);
#else
        Serial.println("UMSeriesD ERROR: getVbusPresent not available");
        return false;
#endif
    }

private:
#if defined(HAS_RGB)
    rmt_data_t rmt_data[3 * 8];
    unsigned long next_rmt_write;
    uint8_t pixel_color[3];
    uint8_t brightness = 0;
#endif

#if defined(ARDUINO_FEATHERS3)
    esp_adc_cal_characteristics_t adc_cal;
#endif

    /* I2C communication for MAX17048 FG*/
    void i2c_write(const MAX17048_REG reg)
    {
        wire->beginTransmission(I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->endTransmission();
    }

    void i2c_write(const MAX17048_REG reg, const uint16_t data)
    {
        wire->beginTransmission(I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->write((data & 0xFF00) >> 8);
        wire->write((data & 0x00FF) >> 0);
        wire->endTransmission();
    }

    uint16_t i2c_read(const MAX17048_REG reg)
    {
        i2c_write(reg);
        wire->requestFrom((uint8_t)I2C_ADDR, (uint8_t)2); // 2byte R/W only
        uint16_t data = (uint16_t)((wire->read() << 8) & 0xFF00);
        data |= (uint16_t)(wire->read() & 0x00FF);
        return data;
    }

    TwoWire *wire;
};

#endif
