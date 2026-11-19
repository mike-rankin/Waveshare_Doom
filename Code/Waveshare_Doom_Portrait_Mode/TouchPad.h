#pragma once

// IDoomInput built from the FT3168 touch panel: a game-pad drawn in the
// strip below Doom's picture.
//
//   +-------------------------------+
//   |          DOOM 368x230         |
//   +-------------------------------+
//   |      [^]       |    FIRE      |
//   |  [<]      [>]  |    USE       |
//   |      [v]       | MENU | ENTER |
//   +-------------------------------+
//
// D-pad: Up/Down = walk forward/back, Left/Right = turn. The whole
// circle around the D-pad is one touch zone, so a finger between two
// arrows gives a diagonal (e.g. walk + turn at once).
// FIRE = shoot, USE = doors/switches, MENU = Esc, ENTER = select.
//
// The FT3168 reports up to two fingers, so you can hold the D-pad with
// one thumb and press FIRE with the other.
//
// ESP-DOOM maps its 8 buttons to Doom keys:
//   Up/Down/Left/Right -> arrows, A -> fire, B -> use,
//   Start -> Enter, Select -> Escape.

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <IDoomInput.h>
#include "hw_conf.h"

class TouchPad : public IDoomInput
{
public:
    explicit TouchPad(Arduino_GFX* gfx) : _gfx(gfx) {}

    void begin() override
    {
        // FocalTech power-mode register: 0 = active (fastest response).
        writeReg(0xA5, 0x00);
        drawAll();
    }

    void update() override
    {
        _previous = _current;
        _current  = readMask();

        // Only redraw buttons whose state changed (cheap, and happens
        // between Doom frames on the same task, so no bus contention).
        uint8_t changed = _current ^ _previous;
        if (changed)
            for (int i = 0; i < NUM_BTN; i++)
                if (changed & (1 << i))
                    drawButton(i, _current & (1 << i));
    }

    bool down(DoomButton b) override     { return  (_current  & mask(b)); }
    bool pressed(DoomButton b) override  { return  (_current  & mask(b)) && !(_previous & mask(b)); }
    bool released(DoomButton b) override { return !(_current  & mask(b)) &&  (_previous & mask(b)); }

    // Redraws every control (call after clearing the screen).
    void drawAll()
    {
        // Thin divider between the game and the controls.
        _gfx->drawFastHLine(24, GAME_Y + GAME_H + 4, AMOLED_W - 48, COL_EDGE);
        for (int i = 0; i < NUM_BTN; i++)
            drawButton(i, false);
    }

private:
    // ---- layout (portrait, 368 x 448) ---------------------------------
    // Index order matches DoomButton: Up, Down, Left, Right, A, B, Start, Select
    struct Btn { int16_t x, y, w, h; const char* label; };

    static constexpr int NUM_BTN = 8;

    // D-pad centre and size
    static constexpr int PAD_CX = 100;
    static constexpr int PAD_CY = 345;
    static constexpr int PAD_B  = 58;   // arrow button size
    static constexpr int PAD_D  = 60;   // centre -> arrow centre
    static constexpr int PAD_ZONE_R = 112;  // touch zone radius
    static constexpr int PAD_DEAD_R = 14;   // dead zone in the middle

    const Btn _btn[NUM_BTN] =
    {
        { PAD_CX - PAD_B/2,         PAD_CY - PAD_D - PAD_B/2, PAD_B, PAD_B, "^" },     // Up
        { PAD_CX - PAD_B/2,         PAD_CY + PAD_D - PAD_B/2, PAD_B, PAD_B, "v" },     // Down
        { PAD_CX - PAD_D - PAD_B/2, PAD_CY - PAD_B/2,         PAD_B, PAD_B, "<" },     // Left
        { PAD_CX + PAD_D - PAD_B/2, PAD_CY - PAD_B/2,         PAD_B, PAD_B, ">" },     // Right
        { 212, 256, 144, 84, "FIRE"  },                                                // A
        { 212, 348, 144, 42, "USE"   },                                                // B
        { 284, 398,  66, 34, "ENTER" },                                                // Start
        { 212, 398,  66, 34, "MENU"  },                                                // Select
    };

    // Dim colours on purpose: these are static pixels on an AMOLED, and
    // dim static content is much kinder to the panel (burn-in).
    static constexpr uint16_t COL_EDGE    = 0x39E7;  // dark grey
    static constexpr uint16_t COL_TEXT    = 0x7BEF;  // mid grey
    static constexpr uint16_t COL_FILL_ON = 0x7800;  // dark red
    static constexpr uint16_t COL_FIRE    = 0x5000;  // very dark red outline for FIRE

    static uint8_t mask(DoomButton b) { return (uint8_t)(1u << (int)b); }

    // ---- drawing ------------------------------------------------------
    void drawButton(int i, bool on)
    {
        const Btn& b = _btn[i];
        const int r = 10;

        _gfx->fillRoundRect(b.x, b.y, b.w, b.h, r, on ? COL_FILL_ON : RGB565_BLACK);
        _gfx->drawRoundRect(b.x, b.y, b.w, b.h, r,
                            (i == (int)DoomButton::A) ? COL_FIRE : COL_EDGE);

        const uint8_t size = (b.h >= 50) ? 3 : 2;
        const int tw = (int)strlen(b.label) * 6 * size;
        const int th = 8 * size;
        _gfx->setTextSize(size);
        _gfx->setTextColor(on ? RGB565_WHITE : COL_TEXT);
        _gfx->setCursor(b.x + (b.w - tw) / 2, b.y + (b.h - th) / 2);
        _gfx->print(b.label);
    }

    // ---- touch --------------------------------------------------------
    uint8_t readMask()
    {
        uint8_t buf[11];   // registers 0x02..0x0C
        if (!readRegs(0x02, buf, sizeof(buf)))
            return 0;      // I2C hiccup: treat as "no touch" this poll

        int points = buf[0] & 0x0F;
        if (points > 2) points = 2;

        uint8_t m = 0;
        for (int p = 0; p < points; p++)
        {
            const uint8_t* d = buf + 1 + p * 6;   // P1 at 0x03, P2 at 0x09
            uint8_t event = d[0] >> 6;            // 0 down, 1 up, 2 contact
            if (event == 1) continue;             // finger lifting
            int x = ((d[0] & 0x0F) << 8) | d[1];
            int y = ((d[2] & 0x0F) << 8) | d[3];
            m |= hitTest(x, y);
        }
        return m;
    }

    uint8_t hitTest(int x, int y)
    {
        uint8_t m = 0;

        // Touches on the game picture itself are ignored.
        if (y < GAME_Y + GAME_H + 6) return 0;

        // D-pad zone: direction from the pad centre, 8-way.
        const int dx = x - PAD_CX;
        const int dy = y - PAD_CY;
        const int d2 = dx * dx + dy * dy;
        if (d2 <= PAD_ZONE_R * PAD_ZONE_R && x < 205)
        {
            if (d2 < PAD_DEAD_R * PAD_DEAD_R) return 0;
            const int ax = abs(dx), ay = abs(dy);
            // tan(22.5 deg) ~= 0.414 -> 8 equal sectors (x1000 to stay integer)
            if (ay * 1000 > ax * 414) m |= mask(dy < 0 ? DoomButton::Up   : DoomButton::Down);
            if (ax * 1000 > ay * 414) m |= mask(dx < 0 ? DoomButton::Left : DoomButton::Right);
            return m;
        }

        // Right-hand buttons: rectangles with a little slop.
        const int slop = 6;
        for (int i = (int)DoomButton::A; i < NUM_BTN; i++)
        {
            const Btn& b = _btn[i];
            if (x >= b.x - slop && x < b.x + b.w + slop &&
                y >= b.y - slop && y < b.y + b.h + slop)
                m |= (uint8_t)(1u << i);
        }
        return m;
    }

    bool readRegs(uint8_t reg, uint8_t* out, size_t len)
    {
        Wire.beginTransmission(TOUCH_I2C_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom((uint8_t)TOUCH_I2C_ADDR, (uint8_t)len) != len) return false;
        for (size_t i = 0; i < len; i++) out[i] = Wire.read();
        return true;
    }

    void writeReg(uint8_t reg, uint8_t val)
    {
        Wire.beginTransmission(TOUCH_I2C_ADDR);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    Arduino_GFX* _gfx;
    uint8_t _current  = 0;
    uint8_t _previous = 0;
};
