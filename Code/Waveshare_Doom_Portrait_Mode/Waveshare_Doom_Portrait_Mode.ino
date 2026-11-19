// ==================================================================
//  DOOM on the Waveshare ESP32-S3-Touch-AMOLED-1.8
//  (original revision: SH8601 display + FT3168 touch)
//
//  Engine:  ESP-DOOM (doomgeneric for Arduino) - Library Manager
//  WAD:     DOOM1.WAD (shareware v1.9) in the root of the microSD card
//  Screen:  Doom at 368x230 on top, touch game-pad underneath
//  Sound:   ES8311 codec -> onboard speaker (SFX + synthesized music)
//
//  Arduino IDE  Tools menu (see README.md for the full list):
//    Board: ESP32S3 Dev Module     PSRAM: OPI PSRAM
//    Flash Size: 16MB              Partition: 16M Flash (3MB APP/9.9MB FATFS)
//    USB CDC On Boot: Enabled      CPU: 240MHz
// ==================================================================

// Install ESP-DOOM from the library

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include "hw_conf.h"
#include "AmoledDisplay.h"
#include "TouchPad.h"
#include "Es8311Output.h"
#include "Power.h"

#include <DoomGlue.h>
#include <DoomSound.h>

extern "C"
{
#include <doomgeneric_fs.h>
#include <doomkeys.h>

// From the engine (m_controls.c / i_system.c), declared here directly
// rather than pulling in their headers.
extern int key_menu_confirm;
void I_AtExit(void (*func)(void), uint32_t run_if_error);
}

static AmoledDisplay display;
static TouchPad*     touchPad = nullptr;
static Es8311Output  audioOut;

// ------------------------------------------------------------------
//  TCA9554 IO expander: releases the display/touch reset lines (P0-P2)
//  and enables P7, exactly as Waveshare's own examples do.
// ------------------------------------------------------------------
static bool expanderWrite(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(EXPANDER_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool boardPowerUp()
{
    // reg 1 = output port, reg 3 = configuration (0 = output).
    if (!expanderWrite(0x01, 0x80)) return false;   // P0-P2 low (reset asserted), P7 high
    expanderWrite(0x03, (uint8_t)~0x87);            // P0, P1, P2, P7 as outputs
    delay(20);
    expanderWrite(0x01, 0x87);   // release resets
    delay(120);                  // let panel + touch controller boot
    return true;
}

// Called by powerOff() just before the PMIC cuts power.
static void screenOff()
{
    Arduino_GFX* g = display.gfx();
    if (!g) return;
    g->fillScreen(RGB565_BLACK);
    ((Arduino_SH8601*)g)->setBrightness(0);
}

// Doom's "Quit Game" ends in I_Quit(), which runs the functions
// registered with I_AtExit(). Ours powers the board off.
static void doomQuitPowerOff()
{
    powerOff();
}

// Shows a message on the panel and stops (used for setup errors).
static void fatal(const char* line1, const char* line2 = nullptr)
{
    Serial.printf("FATAL: %s %s\n", line1, line2 ? line2 : "");
    Arduino_GFX* g = display.gfx();
    if (g)
    {
        g->fillScreen(RGB565_BLACK);
        g->setTextSize(2);
        g->setTextColor(RGB565_RED);
        g->setCursor(16, 180);
        g->println(line1);
        if (line2)
        {
            g->setTextColor(RGB565_WHITE);
            g->setCursor(16, 210);
            g->println(line2);
        }
    }
    while (true) delay(1000);
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Waveshare AMOLED 1.8 DOOM ===");

    // Shared I2C bus: touch, expander, ES8311, PMU, RTC, IMU
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_HZ);

    if (!boardPowerUp())
        Serial.println("TCA9554 not found at 0x20 - continuing anyway");

    pmicEnableCodecRail();   // ES8311 analog supply (see Power.h)

    if (!display.begin())
    {
        Serial.println("Display init failed (or out of RAM for frame buffer)");
        while (true) delay(1000);
    }

    // PSRAM check: ESP-DOOM needs ~6.5 MB free for Doom's zone.
    const size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    Serial.printf("PSRAM free: %u bytes\n", (unsigned)psramFree);
    if (psramFree < 6500000)
        fatal("Not enough PSRAM.", "Tools > PSRAM: OPI PSRAM");

    // microSD (SDMMC, 1-bit)
    SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
    if (!SD_MMC.begin("/sdcard", true /* 1-bit mode */))
        fatal("microSD mount failed.", "FAT32 card inserted?");

    if (!SD_MMC.exists(DOOM_WAD_PATH))
        fatal("doom1.wad not found", "Put it in the card root");

    Serial.printf("SD OK, WAD found: %s\n", DOOM_WAD_PATH);

    // Touch game-pad (draws itself under the game area)
    touchPad = new TouchPad(display.gfx());
    touchPad->begin();

    // Hand everything to ESP-DOOM
    DG_FS      = &SD_MMC;
    DG_Display = &display;
    DG_Input   = touchPad;
    DG_PCM     = &audioOut;   // set to nullptr to run silent
    DoomSound_Init();

    // PWR button: hold ~2 s to power off
    g_powerOffHook = screenOff;
    pmicPowerKeyBegin();

    Serial.println("Starting DOOM...");
    DoomGlue_Begin(DOOM_WAD_PATH);

    // After Doom's init (its config load would overwrite this):
    // confirm Yes/No prompts with the DOWN arrow instead of the 'y' key,
    // which the touch pad doesn't have. Covers Quit Game, End Game,
    // Nightmare skill, etc. MENU (Esc) still cancels.
    key_menu_confirm = KEY_DOWNARROW;

    // Menu > Quit Game > DOWN  ->  power off
    I_AtExit(doomQuitPowerOff, false);
}

void loop()
{
    DoomGlue_Tick();

    if (g_powerOffRequested)
        powerOff();
}
