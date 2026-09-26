/*
 * Unexpected Maker Series[D] Arduino Helper Library
 * Release version 1.0.0
 * Requires ESP32 Arduino Core 3.x - 3.2.1 for EdgeS3[D] support
 * TinyPICO[D] and TinyC6[D] use the TinyPICO and TinyC6 board selections
 */

#ifndef _UMSeriesD_H
#define _UMSeriesD_H

#include <Arduino.h>
#include <Wire.h>

// UM_RGB_DATA, UM_RGB_PWR and UM_VBUS_SENSE are set per board, as the core
// variants for the TinyPICO and TinyC6 describe the original (non [D]) boards
#if defined(ARDUINO_FEATHERS3)
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
#define ALS_ADC_CHANNEL ADC1_GPIO4_CHANNEL
#define ALS_ADC_PIN 4
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define UM_RGB_DATA RGB_DATA
#define UM_RGB_PWR RGB_PWR
#define UM_VBUS_SENSE VBUS_SENSE
#define RF_SWITCH 41
#elif defined(ARDUINO_TINYS3)
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define UM_RGB_DATA RGB_DATA
#define UM_RGB_PWR RGB_PWR
#define UM_VBUS_SENSE VBUS_SENSE
#define RF_SWITCH 38
#elif defined(ARDUINO_PROS3)
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define UM_RGB_DATA RGB_DATA
#define UM_RGB_PWR RGB_PWR
#define UM_VBUS_SENSE VBUS_SENSE
#define RF_SWITCH 11
#elif defined(ARDUINO_EDGES3D)
#define RF_SWITCH 38
#elif defined(ARDUINO_TINYPICO)
// TinyPICO[D]
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define UM_RGB_DATA 2
#define UM_RGB_PWR 13
#define UM_VBUS_SENSE 9
#define RF_SWITCH 12
#elif defined(ARDUINO_TINYC6)
// TinyC6[D]
#define HAS_RGB 1
#define HAS_VBUS_SENSE 1
#define HAS_IO_EXPANDER 1
#define UM_RGB_DATA 23
#define UM_RGB_PWR 22
#define UM_VBUS_SENSE 5
// RF Switch is on FXL6408 IO expander pin XIO0
#define RF_SWITCH 0
#define RF_SWITCH_ON_IO_EXPANDER 1
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

#if defined(HAS_IO_EXPANDER)
    enum class FXL6408_REG
    {
        DEVICE_ID = 0x01,
        IO_DIR = 0x03,
        OUTPUT_STATE = 0x05,
        OUTPUT_HIGH_Z = 0x07,
        INPUT_DEFAULT_STATE = 0x09,
        PULL_ENABLE = 0x0B,
        PULL_UP_DOWN = 0x0D,
        INPUT_STATUS = 0x0F,
        INT_MASK = 0x11,
        INT_STATUS = 0x13
    };

    // FXL6408 address is 0x43 with ADDR tied low, 0x44 with ADDR tied high
#if !defined(FXL6408_I2C_ADDR)
#define FXL6408_I2C_ADDR 0x43
#endif
    const uint8_t IOX_I2C_ADDR = FXL6408_I2C_ADDR;
#endif

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
        pinMode(UM_RGB_PWR, OUTPUT);
        rmtInit(UM_RGB_DATA, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
#endif

#if defined(ARDUINO_FEATHERS3)
        pinMode(LED_BUILTIN, OUTPUT);
#endif

#if defined(ARDUINO_FEATHERS3)
        analogSetPinAttenuation(ALS_ADC_PIN, ADC_11db);
#endif

#if defined(HAS_VBUS_SENSE)
        pinMode(UM_VBUS_SENSE, INPUT);
#endif

#if defined(HAS_IO_EXPANDER)
        // The IO expander is needed from begin(), so start the default Wire
        // if FG_setup() hasn't been called yet. FG_setup() can still replace it.
        if (wire == nullptr)
        {
            Wire.begin();
            wire = &Wire;
        }
        IOX_begin();
#endif

        // Setup the RF Switch IO and set to onboard
#if defined(RF_SWITCH_ON_IO_EXPANDER)
        IOX_digitalWrite(RF_SWITCH, false);
        IOX_pinMode(RF_SWITCH, OUTPUT);
#else
        pinMode(RF_SWITCH, OUTPUT);
        digitalWrite(RF_SWITCH, false);
#endif
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
        digitalWrite(UM_RGB_PWR, on);
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

        rmtWrite(UM_RGB_DATA, rmt_data, 3 * 8, RMT_WAIT_FOR_EVER);
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

    /* FXL6408 8-bit I2C IO expander - uses the TwoWire passed to FG_setup() */
    bool IOX_begin()
    {
#if defined(HAS_IO_EXPANDER)
        // Manufacturer ID is 0b101 in bits 7:5. Reading also clears the reset interrupt flag
        return (iox_read(FXL6408_REG::DEVICE_ID) >> 5) == 0b101;
#else
        Serial.println("UMSeriesD ERROR: IOX_begin not available");
        return false;
#endif
    }

    void IOX_pinMode(uint8_t pin, uint8_t mode)
    {
#if defined(HAS_IO_EXPANDER)
        uint8_t mask = 1 << pin;
        if (mode == OUTPUT)
        {
            iox_update(FXL6408_REG::IO_DIR, mask, true);
            // Outputs default to high-Z after reset, so enable the driver
            iox_update(FXL6408_REG::OUTPUT_HIGH_Z, mask, false);
        }
        else
        {
            iox_update(FXL6408_REG::IO_DIR, mask, false);
            iox_update(FXL6408_REG::PULL_UP_DOWN, mask, mode == INPUT_PULLUP);
            iox_update(FXL6408_REG::PULL_ENABLE, mask, mode == INPUT_PULLUP || mode == INPUT_PULLDOWN);
        }
#else
        Serial.println("UMSeriesD ERROR: IOX_pinMode not available");
#endif
    }

    void IOX_digitalWrite(uint8_t pin, bool state)
    {
#if defined(HAS_IO_EXPANDER)
        iox_update(FXL6408_REG::OUTPUT_STATE, 1 << pin, state);
#else
        Serial.println("UMSeriesD ERROR: IOX_digitalWrite not available");
#endif
    }

    bool IOX_digitalRead(uint8_t pin)
    {
#if defined(HAS_IO_EXPANDER)
        // Reading the input status also clears any pending interrupt
        return (iox_read(FXL6408_REG::INPUT_STATUS) >> pin) & 1;
#else
        Serial.println("UMSeriesD ERROR: IOX_digitalRead not available");
        return false;
#endif
    }

    void setAntennaExternal(bool state)
    {
        // Set the RF Switch HIGH for External and LOW for Internal
#if defined(RF_SWITCH_ON_IO_EXPANDER)
        IOX_digitalWrite(RF_SWITCH, state);
#else
        digitalWrite(RF_SWITCH, state);
#endif
    }

    bool getVbusPresent()
    {
#if defined(HAS_VBUS_SENSE)
        return digitalRead(UM_VBUS_SENSE);
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

#if defined(HAS_IO_EXPANDER)
    /* I2C communication for FXL6408 IO expander */
    void iox_write(const FXL6408_REG reg, const uint8_t data)
    {
        wire->beginTransmission(IOX_I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->write(data);
        wire->endTransmission();
    }

    uint8_t iox_read(const FXL6408_REG reg)
    {
        wire->beginTransmission(IOX_I2C_ADDR);
        wire->write((uint8_t)reg);
        wire->endTransmission(false);
        wire->requestFrom((uint8_t)IOX_I2C_ADDR, (uint8_t)1);
        return wire->read();
    }

    // Read-modify-write the bits in mask to set or clear
    void iox_update(const FXL6408_REG reg, const uint8_t mask, const bool set)
    {
        uint8_t data = iox_read(reg);
        iox_write(reg, set ? (data | mask) : (data & ~mask));
    }
#endif

    TwoWire *wire = nullptr;
};

#endif
