// ==================================================================
//  DOOM on the Waveshare ESP32-S3-Touch-AMOLED-1.8  -  LANDSCAPE
//  (original revision: SH8601 display + FT3168 touch)
//
//  Full-screen 448x336 (Doom's original 4:3 shape), see-through touch
//  buttons over the game, strafe / weapon / map / run, saves to microSD,
//  ES8311 sound, power off by PWR long press or Menu > Quit > DOWN.
//
//  Arduino IDE  Tools menu:
//    Board: ESP32S3 Dev Module     PSRAM: OPI PSRAM
//    Flash Size: 16MB              Partition: 16M Flash (3MB APP/9.9MB FATFS)
//    USB CDC On Boot: Enabled      CPU: 240MHz
//  Libraries: ESP-DOOM, GFX Library for Arduino
// ==================================================================

#include <Arduino.h>
#include <Wire.h>
#include <FS.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include "hw_conf.h"
#include "LandscapeDisplay.h"
#include "TouchOverlay.h"
#include "Es8311Output.h"
#include "Power.h"

#include <DoomGlue.h>
#include <DoomSound.h>

extern "C"
{
#include <doomgeneric_fs.h>

// Doom engine globals / functions (C linkage), declared directly.
extern int   key_menu_confirm;
extern int   key_nextweapon;
extern char* savegamedir;
void I_AtExit(void (*func)(void), uint32_t run_if_error);
}

static LandscapeDisplay display;
static TouchOverlay     controls;
static Es8311Output     audioOut;

// ------------------------------------------------------------------
//  TCA9554 IO expander: release display/touch resets (P0-P2), P7 on
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
    if (!expanderWrite(0x01, 0x80)) return false;   // P0-P2 low (reset), P7 high
    expanderWrite(0x03, (uint8_t)~0x87);            // P0, P1, P2, P7 outputs
    delay(20);
    expanderWrite(0x01, 0x87);                      // release resets
    delay(120);
    return true;
}

// ------------------------------------------------------------------
//  Hooks
// ------------------------------------------------------------------
static void drawOverlay(Arduino_GFX* g) { controls.draw(g); }

static void screenOff()
{
    Arduino_GFX* g = display.gfx();
    if (g) { g->fillScreen(RGB565_BLACK); ((Arduino_Canvas*)g)->flush(); }
    if (display.panel()) ((Arduino_SH8601*)display.panel())->setBrightness(0);
}

static void doomQuitPowerOff() { powerOff(); }

static void fatal(const char* line1, const char* line2 = nullptr)
{
    Serial.printf("FATAL: %s %s\n", line1, line2 ? line2 : "");
    Arduino_GFX* g = display.gfx();
    if (g)
    {
        g->fillScreen(RGB565_BLACK);
        g->setTextSize(2);
        g->setTextColor(RGB565_RED);
        g->setCursor(20, 150);
        g->println(line1);
        if (line2)
        {
            g->setTextColor(RGB565_WHITE);
            g->setCursor(20, 180);
            g->println(line2);
        }
        ((Arduino_Canvas*)g)->flush();
    }
    while (true) delay(1000);
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Waveshare AMOLED 1.8 DOOM (landscape) ===");

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_HZ);

    if (!boardPowerUp())
        Serial.println("TCA9554 not found at 0x20 - continuing anyway");

    pmicEnableCodecRail();   // ES8311 analog supply (Power.h)

    if (!display.begin())
    {
        Serial.println("Display init failed (canvas needs ~330 KB)");
        while (true) delay(1000);
    }

    const size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    Serial.printf("PSRAM free: %u bytes\n", (unsigned)psramFree);
    if (psramFree < 6500000)
        fatal("Not enough PSRAM.", "Tools > PSRAM: OPI PSRAM");

    SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
    if (!SD_MMC.begin("/sdcard", true /* 1-bit */))
        fatal("microSD mount failed.", "FAT32 card inserted?");

    if (!SD_MMC.exists(DOOM_WAD_PATH))
        fatal("doom1.wad not found", "Put it in the card root");

    if (!SD_MMC.exists(SAVE_DIR_SD))
        SD_MMC.mkdir(SAVE_DIR_SD);

    Serial.printf("SD OK, WAD found: %s\n", DOOM_WAD_PATH);

    controls.begin();
    display.setOverlay(drawOverlay);

    DG_FS      = &SD_MMC;
    DG_Display = &display;
    DG_Input   = &controls;
    DG_PCM     = &audioOut;
    DoomSound_Init();

    g_powerOffHook = screenOff;
    pmicPowerKeyBegin();

    Serial.println("Starting DOOM...");
    DoomGlue_Begin(DOOM_WAD_PATH);

    // ---- after Doom's init (its config load would overwrite these) ----
    key_menu_confirm = KEY_DOWNARROW;        // Yes/No prompts: DOWN = yes
    key_nextweapon   = ']';                  // WPN button
    controls.setRun(RUN_DEFAULT_ON);

    // Save games to the card. Must be set before the first Save/Load menu.
    savegamedir = (char*)SAVE_DIR_VFS;

    I_AtExit(doomQuitPowerOff, false);       // Quit Game -> power off
}

void loop()
{
    DoomGlue_Tick();

    if (g_powerOffRequested)
        powerOff();
}
