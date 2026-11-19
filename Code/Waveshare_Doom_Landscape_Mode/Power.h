#pragma once

// AXP2101 PMIC helpers:
//   - ALDO1 on (ES8311 analog supply)
//   - PWR button: hold ~2 s -> power off
//   - powerOff(): amp off, screen dark, PMIC cuts power
//
// AXP2101 registers used (I2C 0x34):
//   0x10 bit0      : write 1 -> PMIC powers the board OFF
//   0x27           : PWR key timing. [5:4] long-press time (1/1.5/2/2.5 s),
//                    [3:2] hardware force-off time (4/6/8/10 s), [1:0] power-on hold
//   0x41           : IRQ enables (bit3 short press, bit2 long press, bit1 key down)
//   0x49           : IRQ status, write 1 to clear (same bits as 0x41)
//   0x90 / 0x92    : LDO enables / ALDO1 voltage

#include <Arduino.h>
#include <Wire.h>
#include "hw_conf.h"

// ---------------- I2C register helpers ----------------
static inline bool i2cRead8(uint8_t addr, uint8_t reg, uint8_t& val)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
    val = Wire.read();
    return true;
}

static inline bool i2cWrite8(uint8_t addr, uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

// ---------------- ALDO1 (ES8311 analog supply) ----------------
// The PMIC keeps rail settings across resets/reflashes while it has
// power, so a previous firmware (e.g. pocket-tank) can leave ALDO1 off.
static inline void pmicEnableCodecRail()
{
    uint8_t en = 0;
    if (!i2cRead8(PMIC_I2C_ADDR, 0x90, en))
    {
        Serial.println("[pmic] AXP2101 not responding at 0x34");
        return;
    }
    Serial.printf("[pmic] LDO enables (0x90) before: 0x%02X  ALDO1 %s\n", en, (en & 0x01) ? "ON" : "OFF");

    i2cWrite8(PMIC_I2C_ADDR, 0x92, 0x1C);        // ALDO1 = 500 mV + 28 * 100 mV = 3.3 V
    i2cWrite8(PMIC_I2C_ADDR, 0x90, en | 0x01);   // ALDO1 on, other rails untouched
    delay(10);

    i2cRead8(PMIC_I2C_ADDR, 0x90, en);
    Serial.printf("[pmic] LDO enables (0x90) after:  0x%02X  ALDO1 %s\n", en, (en & 0x01) ? "ON" : "OFF");

    uint8_t id;
    Serial.printf("[audio] ES8311 at 0x18: %s\n", i2cRead8(0x18, 0xFD, id) ? "found" : "NOT FOUND");
}

// ---------------- Power off ----------------
// Hook for the sketch to blank the screen before power is cut.
typedef void (*PowerOffHook)();
static PowerOffHook g_powerOffHook = nullptr;

[[noreturn]] static inline void powerOff()
{
    Serial.println("[pmic] Powering off...");
    digitalWrite(AUDIO_PIN_PA, LOW);             // speaker amp off (no pop)
    if (g_powerOffHook) g_powerOffHook();        // screen dark
    delay(50);

    uint8_t v = 0;
    i2cRead8(PMIC_I2C_ADDR, 0x10, v);
    i2cWrite8(PMIC_I2C_ADDR, 0x10, v | 0x01);    // PMIC: power OFF

    // If we get here, the PMIC didn't cut power (e.g. kept alive by USB).
    delay(500);
    Serial.println("[pmic] Still powered (USB?). Restarting instead.");
    esp_restart();
    while (true) {}
}

// ---------------- PWR key long press ----------------
// Polled from a small background task. It only sets a flag; the main
// loop does the actual power-off so the display isn't touched from two
// tasks at once.
static volatile bool g_powerOffRequested = false;

static void pmicKeyTask(void*)
{
    const uint32_t EDGE_WINDOW_MS = 4000;
    uint32_t lastKeyDown = 0;
    bool sawKeyDown = false;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        uint8_t st = 0;
        if (!i2cRead8(PMIC_I2C_ADDR, 0x49, st)) continue;
        if (!(st & 0x0E)) continue;

        i2cWrite8(PMIC_I2C_ADDR, 0x49, st & 0x0E);   // clear what we saw

        if (st & 0x02) { sawKeyDown = true; lastKeyDown = millis(); }

        // Only accept a long press that followed a real key-down edge
        // (guards against stray long-press flags, seen on USB power).
        if ((st & 0x04) && sawKeyDown && (millis() - lastKeyDown) < EDGE_WINDOW_MS)
        {
            Serial.println("[pmic] PWR long press");
            g_powerOffRequested = true;
        }
    }
}

static inline void pmicPowerKeyBegin()
{
    uint8_t v = 0;

    // Long press = 2 s ([5:4]=10), hardware force-off backup = 6 s ([3:2]=01),
    // keep the power-on hold time [1:0] and upper bits as they are.
    if (i2cRead8(PMIC_I2C_ADDR, 0x27, v))
        i2cWrite8(PMIC_I2C_ADDR, 0x27, (uint8_t)((v & 0xC3) | (0x2 << 4) | (0x1 << 2)));

    // Enable short/long press + key-down IRQ flags (polled, no IRQ pin needed)
    if (i2cRead8(PMIC_I2C_ADDR, 0x41, v))
        i2cWrite8(PMIC_I2C_ADDR, 0x41, v | 0x0E);

    // Clear the press that powered the board on
    i2cWrite8(PMIC_I2C_ADDR, 0x49, 0xFF);

    xTaskCreatePinnedToCore(pmicKeyTask, "pwr_key", 3072, nullptr, 1, nullptr, 0);
    Serial.println("[pmic] PWR key: hold 2 s to power off");
}
