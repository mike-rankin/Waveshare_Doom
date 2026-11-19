# DOOM on Waveshare ESP32-S3-Touch-AMOLED-1.8 (SH8601 / FT3168)

## 1. Libraries (Arduino IDE > Library Manager)
- **ESP-DOOM** by Rear Labs (the Doom engine)
- **GFX Library for Arduino** by Moon On Our Nation (needed by ESP-DOOM and for the SH8601 driver)

Board package: **esp32 by Espressif Systems 3.x** (Waveshare tests with 3.3.x).

## 2. Tools menu
| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | **OPI PSRAM** |
| Upload Speed | 921600 |

## 3. microSD card
- Format FAT32.
- Copy `DOOM1.WAD` (shareware v1.9) to the **root** of the card. Upper/lower case doesn't matter.

## 4. Files in this sketch folder
| File | What it does |
|---|---|
| `Waveshare_AMOLED_1_8_Doom.ino` | Board bring-up (I2C, IO expander, SD), hands everything to ESP-DOOM |
| `hw_conf.h` | All pins and settings in one place |
| `AmoledDisplay.h` | Scales Doom's 320x200 to 368x230 and pushes it over QSPI |
| `TouchPad.h` | On-screen game-pad on the FT3168 |
| `Es8311Output.h` | Doom audio -> I2S -> ES8311 -> speaker |
| `Power.h` | AXP2101: codec power rail, PWR-button long press, power off |
| `es8311.c/.h`, `es8311_reg.h` | ES8311 driver (copied from Waveshare's 15_ES8311 example, Apache-2.0) |

## 5. Controls
```
+-------------------------------+
|          DOOM 368x230         |
+-------------------------------+
|      [^]       |    FIRE      |
|  [<]      [>]  |    USE       |
|      [v]       | MENU | ENTER |
+-------------------------------+
```
- D-pad: up/down = walk, left/right = turn. Touch between arrows for diagonals (walk + turn).
- Two fingers work: hold the D-pad and press FIRE together.
- MENU = Esc (main menu / back), ENTER = select. Weapons switch automatically when you pick them up.
- Yes/No prompts (Quit, End Game, Nightmare): press **DOWN** for yes, MENU for no. The text on screen still says "press y"; that's built into the engine.

## Power
- **Power off:** hold the PWR button for about 2 s, or choose MENU > Quit Game and press DOWN.
- **Power on:** short press of PWR.
- If the PMIC won't cut power (for example while USB is connected), the board restarts instead.
- Backup: holding PWR for 6 s makes the PMIC itself force power off, even if the firmware is frozen.

## 6. Tuning (hw_conf.h)
- `AUDIO_VOLUME` 0-100
- `AMOLED_BRIGHTNESS` 0-255
- `AMOLED_QSPI_HZ` try 80000000 for a faster frame push
- Comment out `DOOM_MUSIC_SYNTH` for sound effects only

## 7. Troubleshooting
- **"Not enough PSRAM"**: PSRAM isn't set to *OPI PSRAM*.
- **"microSD mount failed"**: card must be FAT32; reseat it.
- **Sketch too big**: partition scheme isn't the 3MB APP one.
- **No sound at all**: the sketch turns on the AXP2101's ALDO1 rail (the ES8311's analog supply) at boot. Serial shows `[pmic] ... ALDO1 ON` and `[audio] ES8311 running at 11025 Hz`. Other firmware (e.g. pocket-tank) switches ALDO1 off, and the PMIC remembers that across reflashes.
- **Sound effects play but no music**: the library build isn't picking up `hw_conf.h`. Add `#define DOOM_MUSIC_SYNTH` as the first line of the library's `src/DoomSound.h`.
- **Touch is offset or mirrored**: open Serial Monitor; check that touch X/Y match the screen (Waveshare's 02_Drawing_board example shows raw values).
