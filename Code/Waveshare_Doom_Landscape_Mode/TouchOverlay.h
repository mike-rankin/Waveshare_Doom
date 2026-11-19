#pragma once

// Landscape touch controls drawn OVER the game picture.
//
//  +------------------------------------------------+
//  | [MENU]                                  [MAP]  |
//  |                                                |
//  | [<S] [S>]                                      |
//  |                                        [RUN]   |
//  |      [^]                     [WPN]             |
//  |  [<]     [>]                 [USE]   [ FIRE ]  |
//  |      [v]                                       |
//  +------------------------------------------------+
//
//   D-pad      walk / turn (touch between arrows for diagonals)
//   <S  S>     strafe left / right
//   FIRE       shoot   (acts as ENTER while a menu is open)
//   USE        doors, switches
//   WPN        next weapon
//   RUN        always-run on/off (green = on)
//   MAP        automap
//   MENU       Esc
//
// Unlike the portrait sketch, this one posts key events straight into
// Doom's event queue (D_PostEvent), so it isn't limited to ESP-DOOM's
// 8 buttons. The IDoomInput interface is still implemented (ESP-DOOM
// calls update() every few ms), but it reports nothing itself.

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <IDoomInput.h>
#include "hw_conf.h"
#include "LandscapeDisplay.h"

extern "C"
{
#include <doomkeys.h>

// Doom engine globals / functions (C linkage), declared directly.
struct DoomEvent { int type; int data1, data2, data3, data4; };   // = event_t
void D_PostEvent(DoomEvent* ev);

extern uint32_t menuactive;        // doom_boolean
extern int      joybspeed;         // >= 20 means "always run"
extern int      saveStringEnter;   // typing a save-game name
extern int      saveSlot;
extern int      saveCharIndex;
extern char     savegamestrings[10][24];
extern int      gameepisode;
extern int      gamemap;
extern int      gamemode;          // 2 = commercial (Doom II)
}

class TouchOverlay : public IDoomInput
{
public:
    // ---- IDoomInput (ESP-DOOM polls this) -----------------------------
    void begin() override
    {
        Wire.beginTransmission(TOUCH_I2C_ADDR);   // FT3168: active mode
        Wire.write(0xA5);
        Wire.write(0x00);
        Wire.endTransmission();
    }

    void update() override
    {
        const uint16_t now = readMask();
        const uint16_t changed = now ^ _held;

        for (int i = 0; i < NUM_BTN; i++)
        {
            const uint16_t bitI = (uint16_t)(1u << i);
            if (!(changed & bitI)) continue;
            const bool down = now & bitI;

            if (i == B_RUN)
            {
                if (down) setRun(!_runOn);          // toggle on press
                continue;
            }

            if (down)
            {
                int key = _btn[i].key;
                if (i == B_FIRE && menuactive) key = KEY_ENTER;   // FIRE = ENTER in menus
                _sent[i] = key;
                post(0 /* ev_keydown */, key);
            }
            else
            {
                post(1 /* ev_keyup */, _sent[i]);
            }
        }
        _held = now;

        autoNameSave();
    }

    // Everything goes through D_PostEvent; nothing for ESP-DOOM's own map.
    bool down(DoomButton) override     { return false; }
    bool pressed(DoomButton) override  { return false; }
    bool released(DoomButton) override { return false; }

    // ---- run toggle ----------------------------------------------------
    void setRun(bool on)
    {
        _runOn = on;
        joybspeed = on ? 29 : 2;   // 29 >= MAX_JOY_BUTTONS -> always run
    }

    // ---- drawing (called every frame, after Doom's picture) -------------
    void draw(Arduino_GFX* g)
    {
        // Black bars above/below the game (keeps them clean)
        g->fillRect(0, 0, SCREEN_W, GAME_Y, RGB565_BLACK);
        g->fillRect(0, GAME_Y + GAME_H, SCREEN_W, SCREEN_H - GAME_Y - GAME_H, RGB565_BLACK);

        for (int i = 0; i < NUM_BTN; i++)
        {
            const Btn& b = _btn[i];
            const bool lit = (i == B_RUN) ? _runOn : (_held & (1u << i));
            const uint16_t fill = (i == B_RUN) ? OVERLAY_RUN_ON : OVERLAY_PRESS;
            const int r = 8;

            if (lit) g->fillRoundRect(b.x, b.y, b.w, b.h, r, fill);
            g->drawRoundRect(b.x, b.y, b.w, b.h, r, OVERLAY_EDGE);
            g->drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, r - 1, OVERLAY_SHADOW);

            const uint8_t size = (b.h >= 50 && strlen(b.label) <= 4) ? 2 : 1;
            const int tw = (int)strlen(b.label) * 6 * size;
            const int th = 8 * size;
            g->setTextSize(size);
            g->setTextColor(lit ? RGB565_WHITE : OVERLAY_TEXT);
            g->setCursor(b.x + (b.w - tw) / 2, b.y + (b.h - th) / 2);
            g->print(b.label);
        }
    }

    // Exposed for host testing
    uint16_t hitTest(int x, int y) const
    {
        uint16_t m = 0;

        // Rectangle buttons first (with a little slop)
        const int slop = 6;
        for (int i = B_FIRE; i < NUM_BTN; i++)
        {
            const Btn& b = _btn[i];
            if (x >= b.x - slop && x < b.x + b.w + slop &&
                y >= b.y - slop && y < b.y + b.h + slop)
                return (uint16_t)(1u << i);
        }

        // D-pad zone: 8-way direction from the pad centre
        const int dx = x - PAD_CX, dy = y - PAD_CY;
        const int d2 = dx * dx + dy * dy;
        if (d2 <= PAD_ZONE_R * PAD_ZONE_R && x < 205)
        {
            if (d2 < PAD_DEAD_R * PAD_DEAD_R) return 0;
            const int ax = abs(dx), ay = abs(dy);
            if (ay * 1000 > ax * 414) m |= (uint16_t)(1u << (dy < 0 ? B_UP : B_DOWN));
            if (ax * 1000 > ay * 414) m |= (uint16_t)(1u << (dx < 0 ? B_LEFT : B_RIGHT));
        }
        return m;
    }

    uint16_t held() const { return _held; }
    bool runOn() const { return _runOn; }

private:
    enum { B_UP, B_DOWN, B_LEFT, B_RIGHT, B_FIRE, B_USE, B_WPN,
           B_STRL, B_STRR, B_MAP, B_MENU, B_RUN, NUM_BTN };

    struct Btn { int16_t x, y, w, h; const char* label; int key; };

    // D-pad geometry (logical landscape coordinates, 448 x 368)
    static constexpr int PAD_CX = 92;
    static constexpr int PAD_CY = 268;
    static constexpr int PAD_ZONE_R = 100;
    static constexpr int PAD_DEAD_R = 14;

    const Btn _btn[NUM_BTN] =
    {
        {  65, 185, 54, 54, "^",    KEY_UPARROW    },  // B_UP
        {  65, 297, 54, 54, "v",    KEY_DOWNARROW  },  // B_DOWN
        {   9, 241, 54, 54, "<",    KEY_LEFTARROW  },  // B_LEFT
        { 121, 241, 54, 54, ">",    KEY_RIGHTARROW },  // B_RIGHT
        { 336, 232, 96, 112,"FIRE", KEY_FIRE       },  // B_FIRE
        { 262, 278, 66, 66, "USE",  KEY_USE        },  // B_USE
        { 262, 206, 66, 64, "WPN",  ']'            },  // B_WPN  (key_nextweapon set to ']')
        {   8, 118, 76, 44, "<S",   KEY_STRAFE_L   },  // B_STRL
        { 100, 118, 76, 44, "S>",   KEY_STRAFE_R   },  // B_STRR
        { 368,  24, 64, 40, "MAP",  KEY_TAB        },  // B_MAP
        {  16,  24, 64, 40, "MENU", KEY_ESCAPE     },  // B_MENU
        { 360, 176, 72, 48, "RUN",  0              },  // B_RUN  (toggle)
    };

    // ---- touch read (FT3168, up to 2 fingers) ---------------------------
    uint16_t readMask()
    {
        uint8_t buf[11];
        Wire.beginTransmission(TOUCH_I2C_ADDR);
        Wire.write(0x02);
        if (Wire.endTransmission(false) != 0) return _held;
        if (Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)sizeof(buf)) != sizeof(buf)) return _held;
        for (size_t i = 0; i < sizeof(buf); i++) buf[i] = Wire.read();

        int points = buf[0] & 0x0F;
        if (points > 2) points = 2;

        uint16_t m = 0;
        for (int p = 0; p < points; p++)
        {
            const uint8_t* d = buf + 1 + p * 6;
            if ((d[0] >> 6) == 1) continue;              // lift-off event
            const int tx = ((d[0] & 0x0F) << 8) | d[1];
            const int ty = ((d[2] & 0x0F) << 8) | d[3];
            int lx, ly;
            LandscapeDisplay::touchToLogical(tx, ty, lx, ly);
            m |= hitTest(lx, ly);
        }
        return m;
    }

    static void post(int type, int key)
    {
        if (!key) return;
        DoomEvent ev = { type, key, (type == 0) ? key : 0, 0, 0 };
        D_PostEvent(&ev);
    }

    // Doom won't save a game with an empty name, and there's no keyboard.
    // When you pick an empty save slot, fill in the level name (e.g.
    // "E1M3"); press FIRE (= ENTER) to save.
    void autoNameSave()
    {
        if (!saveStringEnter) return;
        if (saveSlot < 0 || saveSlot > 9) return;
        char* s = savegamestrings[saveSlot];
        if (s[0]) return;
        if (gamemode == 2) snprintf(s, 24, "MAP%02d", gamemap);
        else               snprintf(s, 24, "E%dM%d", gameepisode, gamemap);
        saveCharIndex = (int)strlen(s);
    }

    uint16_t _held  = 0;
    int      _sent[NUM_BTN] = {0};
    bool     _runOn = false;
};
