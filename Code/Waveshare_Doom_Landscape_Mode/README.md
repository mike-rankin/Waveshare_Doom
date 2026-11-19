# DOOM on Waveshare ESP32-S3-Touch-AMOLED-1.8 — LANDSCAPE

Full-screen 448x336 picture (Doom's original 4:3 shape), see-through buttons over the game, extra controls, and saves to the microSD card.

Setup is the same as the portrait sketch: ESP-DOOM + GFX Library for Arduino, OPI PSRAM, 16MB flash, "16M Flash (3MB APP/9.9MB FATFS)", USB CDC On Boot enabled, `DOOM1.WAD` in the card root.

## Controls
```
+------------------------------------------------+
| [MENU]                                  [MAP]  |
| [<S] [S>]                                      |
|                                        [RUN]   |
|      [^]                     [WPN]             |
|  [<]     [>]                 [USE]   [ FIRE ]  |
|      [v]                                       |
+------------------------------------------------+
```
| Button | Action |
|---|---|
| D-pad | walk / turn; touch between arrows for diagonals |
| <S / S> | strafe left / right |
| FIRE | shoot; works as ENTER while a menu is open |
| USE | doors, switches |
| WPN | next weapon |
| RUN | always-run on/off (green = on, starts on) |
| MAP | automap |
| MENU | Esc (menu / back) |

Two fingers work, e.g. D-pad + FIRE.
Yes/No prompts: **DOWN** = yes, MENU = no.

## Saving
- MENU > Save Game > pick a slot. An empty slot is named after the level automatically (e.g. `E1M3`). Press **FIRE** to save.
- Saves go to `/doomsave/` on the microSD card (`doomsav0.dsg` …), so they survive power-off and reflashing.
- MENU > Load Game to continue.

## Power
- Hold PWR about 2 s, or MENU > Quit Game > DOWN, to power off. A short PWR press powers on.

## Settings (hw_conf.h)
- `SCREEN_ROTATION` 1 or 3: flip if the picture is upside down for how you hold it.
- `AMOLED_QSPI_HZ`: 80000000 gives a faster frame push if the panel accepts it.
- `OVERLAY_*` colours, `RUN_DEFAULT_ON`, `AUDIO_VOLUME`, `AMOLED_BRIGHTNESS`.

## Notes
- Each frame now pushes the whole panel (~330 KB), so it runs a little slower than the portrait version.
- Menu settings (volume, screen size) still aren't remembered after power-off. Doom's config file isn't redirected to the card, only the saves.
