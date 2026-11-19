#pragma once

// IDoomDisplay, landscape, full-screen.
//
// A 368x448 Arduino_Canvas (PSRAM) holds the whole panel image. It is
// set to rotation 1 or 3, so anything drawn through it with the normal
// GFX calls (the button overlay) lands in landscape coordinates.
//
// Doom's 320x200 rows are scaled to 448x336 and written straight into
// the canvas memory with the same rotation maths the canvas uses
// (much faster than drawPixel). Each frame: Doom picture -> overlay
// drawn on top -> one flush() of the whole panel.

#include <Arduino_GFX_Library.h>
#include <IDoomDisplay.h>
#include <string.h>
#include "hw_conf.h"

class LandscapeDisplay : public IDoomDisplay
{
public:
    static constexpr int SRC_W = 320;
    static constexpr int SRC_H = 200;

    typedef void (*OverlayFn)(Arduino_GFX* g);

    Arduino_GFX* gfx()    { return _canvas; }   // landscape drawing surface
    Arduino_GFX* panel()  { return _panel; }    // the real panel (brightness etc.)
    void setOverlay(OverlayFn fn) { _overlay = fn; }

    bool begin() override
    {
        if (_canvas) return true;

        _bus   = new Arduino_ESP32QSPI(AMOLED_PIN_CS, AMOLED_PIN_SCLK,
                                       AMOLED_PIN_D0, AMOLED_PIN_D1,
                                       AMOLED_PIN_D2, AMOLED_PIN_D3);
        _panel = new Arduino_SH8601(_bus, GFX_NOT_DEFINED, 0, AMOLED_W, AMOLED_H);
        _canvas = new Arduino_Canvas(AMOLED_W, AMOLED_H, _panel, 0, 0, SCREEN_ROTATION);

        if (!_canvas->begin(AMOLED_QSPI_HZ))   // also starts the panel
            return false;

        _fb = _canvas->getFramebuffer();
        if (!_fb) return false;

        _canvas->fillScreen(RGB565_BLACK);
        _canvas->flush();
        ((Arduino_SH8601*)_panel)->setBrightness(AMOLED_BRIGHTNESS);

        buildTables();
        return true;
    }

    void beginFrame() override {}

    // Native mode: the library sends x=0, 320 pixels, y in 0..199.
    void writeRow(int x, int y, const uint16_t* rgb565, int count) override
    {
        if (x != 0 || count < SRC_W || y < 0 || y >= SRC_H) return;

        for (int r = 0; r < _rowCount[y]; r++)
        {
            const int ly = GAME_Y + _rowFirst[y] + r;      // logical row on screen
            uint16_t* p  = _fb + rowStart(ly);
            const int step = colStep();
            for (int lx = 0; lx < GAME_W; lx++, p += step)
                *p = rgb565[_xLUT[lx]];
        }
    }

    void endFrame() override
    {
        if (_overlay) _overlay(_canvas);
        _canvas->flush();
    }

    int width()  const override { return SRC_W; }
    int height() const override { return SRC_H; }

    // Touch panel reports native portrait coordinates; convert to landscape.
    static void touchToLogical(int tx, int ty, int& lx, int& ly)
    {
#if SCREEN_ROTATION == 1
        lx = ty;
        ly = (AMOLED_W - 1) - tx;
#else
        lx = (AMOLED_H - 1) - ty;
        ly = tx;
#endif
    }

private:
    // Same mapping as Arduino_Canvas::writePixelPreclipped for rotation 1/3:
    //   rot 1: idx = lx * 368 + (367 - ly)
    //   rot 3: idx = (447 - lx) * 368 + ly
    static inline int32_t rowStart(int ly)
    {
#if SCREEN_ROTATION == 1
        return (AMOLED_W - 1) - ly;                                  // lx = 0
#else
        return (int32_t)(AMOLED_H - 1) * AMOLED_W + ly;              // lx = 0
#endif
    }
    static inline int colStep()
    {
#if SCREEN_ROTATION == 1
        return AMOLED_W;
#else
        return -AMOLED_W;
#endif
    }

    void buildTables()
    {
        for (int lx = 0; lx < GAME_W; lx++)
            _xLUT[lx] = (uint16_t)((lx * SRC_W) / GAME_W);

        memset(_rowCount, 0, sizeof(_rowCount));
        for (int dy = 0; dy < GAME_H; dy++)
        {
            int sy = (dy * SRC_H) / GAME_H;
            if (_rowCount[sy] == 0) _rowFirst[sy] = (uint16_t)dy;
            _rowCount[sy]++;
        }
    }

    Arduino_DataBus* _bus    = nullptr;
    Arduino_GFX*     _panel  = nullptr;
    Arduino_Canvas*  _canvas = nullptr;
    uint16_t*        _fb     = nullptr;
    OverlayFn        _overlay = nullptr;

    uint16_t _xLUT[GAME_W];
    uint16_t _rowFirst[SRC_H];
    uint8_t  _rowCount[SRC_H];
};
