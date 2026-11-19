#pragma once

// ==================================================================
//  Waveshare ESP32-S3-Touch-AMOLED-1.8  (ORIGINAL revision)
//  Display: SH8601 (QSPI, 368x448)   Touch: FT3168 (I2C 0x38)
//  Audio:   ES8311 codec + PA        Storage: microSD (SDMMC 1-bit)
//  Pins taken from Waveshare's Arduino examples (Mylibrary/pin_config.h)
// ==================================================================
//
// ESP-DOOM's library sources also read this file (for DOOM_MUSIC_SYNTH).
// Keep it to plain #defines so it is safe from both C and C++.
//
// NOTE: DOOM_PANEL_W/H and DOOM_VIEWPORT_* are deliberately NOT defined.
// The library then hands us Doom's native 320x200 rows and
// AmoledDisplay.h does the scaling itself, so the result is identical
// whether or not the library build can see this file.

// ---------------- Display (QSPI) ----------------
#define AMOLED_PIN_CS      12
#define AMOLED_PIN_SCLK    11
#define AMOLED_PIN_D0       4
#define AMOLED_PIN_D1       5
#define AMOLED_PIN_D2       6
#define AMOLED_PIN_D3       7
#define AMOLED_W          368
#define AMOLED_H          448
#define AMOLED_QSPI_HZ    40000000   // try 80000000 if the panel is happy with it
#define AMOLED_BRIGHTNESS 255        // 0-255

// Where Doom's picture goes on the portrait screen.
// 320x200 scaled x1.15 -> 368x230, aspect ratio preserved.
#define GAME_X              0
#define GAME_Y             12
#define GAME_W            368
#define GAME_H            230

// ---------------- Shared I2C bus ----------------
// FT3168 touch, TCA9554 IO expander, ES8311, AXP2101, RTC, IMU
#define BOARD_I2C_SDA      15
#define BOARD_I2C_SCL      14
#define BOARD_I2C_HZ   400000

#define TOUCH_I2C_ADDR   0x38   // FT3168
#define TOUCH_PIN_INT      21   // not used (we poll), kept for reference
#define EXPANDER_I2C_ADDR 0x20  // TCA9554
#define PMIC_I2C_ADDR    0x34   // AXP2101 (ALDO1 = ES8311 analog supply)

// ---------------- Audio (ES8311) ----------------
#define AUDIO_PIN_MCLK     16
#define AUDIO_PIN_BCLK      9
#define AUDIO_PIN_WS       45
#define AUDIO_PIN_DOUT      8   // ESP32 -> ES8311 DSDIN
#define AUDIO_PIN_PA       46   // speaker amplifier enable
#define AUDIO_VOLUME       65   // 0-100, ES8311 DAC volume

// Synthesized MUS music (chiptune style) + small-speaker EQ/limiter.
// Comment out for sound effects only (saves a little CPU on core 0).
#define DOOM_MUSIC_SYNTH

// ---------------- microSD (SDMMC 1-bit) ----------------
#define SD_PIN_CLK          2
#define SD_PIN_CMD          1
#define SD_PIN_D0           3

// ---------------- WAD ----------------
// FAT lookups are case-insensitive, so DOOM1.WAD on the card is fine.
#define DOOM_WAD_PATH  "/doom1.wad"
