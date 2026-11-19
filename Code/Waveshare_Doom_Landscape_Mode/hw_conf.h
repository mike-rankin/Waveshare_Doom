#pragma once

// ==================================================================
//  Waveshare ESP32-S3-Touch-AMOLED-1.8  (ORIGINAL revision)
//  LANDSCAPE build: Doom full-screen 448x336 (4:3), buttons over the game
//  Display: SH8601 (QSPI, 368x448 native portrait)   Touch: FT3168
// ==================================================================
//
// ESP-DOOM's library sources also read this file (for DOOM_MUSIC_SYNTH).
// Keep it to plain #defines so it is safe from both C and C++.
// DOOM_PANEL_W/H and DOOM_VIEWPORT_* are deliberately NOT defined:
// LandscapeDisplay.h does the scaling and rotation itself.

// ---------------- Display (QSPI) ----------------
#define AMOLED_PIN_CS      12
#define AMOLED_PIN_SCLK    11
#define AMOLED_PIN_D0       4
#define AMOLED_PIN_D1       5
#define AMOLED_PIN_D2       6
#define AMOLED_PIN_D3       7
#define AMOLED_W          368      // native (portrait) panel size
#define AMOLED_H          448
#define AMOLED_QSPI_HZ    40000000 // try 80000000 for a faster frame push
#define AMOLED_BRIGHTNESS 255      // 0-255

// Landscape orientation: 1 or 3. If the picture is upside down for the
// way you hold the board, change 1 <-> 3 (touch follows automatically).
#define SCREEN_ROTATION     1

// Logical (landscape) screen is 448 x 368.
#define SCREEN_W          448
#define SCREEN_H          368

// Doom 320x200 scaled to 448x336 = the original 4:3 picture shape.
// 16 px black bars top and bottom.
#define GAME_X              0
#define GAME_Y             16
#define GAME_W            448
#define GAME_H            336

// ---------------- Overlay buttons ----------------
#define OVERLAY_EDGE    0xC618   // light grey outline
#define OVERLAY_SHADOW  0x0000   // dark inner outline (contrast on bright scenes)
#define OVERLAY_TEXT    0xC618
#define OVERLAY_PRESS   0xA000   // red fill while held
#define OVERLAY_RUN_ON  0x03E0   // green fill when RUN is on
#define RUN_DEFAULT_ON  1        // start with always-run on (1) or off (0)

// ---------------- Shared I2C bus ----------------
#define BOARD_I2C_SDA      15
#define BOARD_I2C_SCL      14
#define BOARD_I2C_HZ   400000

#define TOUCH_I2C_ADDR   0x38   // FT3168
#define TOUCH_PIN_INT      21   // not used (polled)
#define EXPANDER_I2C_ADDR 0x20  // TCA9554
#define PMIC_I2C_ADDR    0x34   // AXP2101 (ALDO1 = ES8311 analog supply)

// ---------------- Audio (ES8311) ----------------
#define AUDIO_PIN_MCLK     16
#define AUDIO_PIN_BCLK      9
#define AUDIO_PIN_WS       45
#define AUDIO_PIN_DOUT      8
#define AUDIO_PIN_PA       46
#define AUDIO_VOLUME       65   // 0-100

#define DOOM_MUSIC_SYNTH

// ---------------- microSD (SDMMC 1-bit) ----------------
#define SD_PIN_CLK          2
#define SD_PIN_CMD          1
#define SD_PIN_D0           3

// ---------------- WAD & saves ----------------
#define DOOM_WAD_PATH   "/doom1.wad"
#define SAVE_DIR_SD     "/doomsave"            // folder on the card
#define SAVE_DIR_VFS    "/sdcard/doomsave/"    // same folder, as Doom's fopen() sees it
