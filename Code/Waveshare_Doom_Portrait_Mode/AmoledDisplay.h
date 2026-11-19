#pragma once

// IDoomDisplay for the SH8601 QSPI AMOLED.
//
// ESP-DOOM gives us Doom's native 320x200 frame one row at a time
// (already converted to RGB565). We scale each row to 368 wide,
// duplicate rows where needed to reach 230 tall, collect the result in
// a 368x230 buffer, then push only that rectangle to the panel in
// endFrame(). The touch controls below the game are never redrawn by
// this class, so they cost nothing per frame.

#include <Arduino_GFX_Library.h>
#include <IDoomDisplay.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "hw_conf.h"

class AmoledDisplay : public IDoomDisplay
{
public:
    static constexpr int SRC_W = 320;
    static constexpr int SRC_H = 200;

    // Lets the sketch and the touch pad draw on the same panel.
    Arduino_GFX* gfx() { return _gfx; }

    bool begin() override
    {
        if (_gfx) return true;   // already started (Doom restart path)

        _bus = new Arduino_ESP32QSPI(AMOLED_PIN_CS, AMOLED_PIN_SCLK,
                                     AMOLED_PIN_D0, AMOLED_PIN_D1,
                                     AMOLED_PIN_D2, AMOLED_PIN_D3);

        _gfx = new Arduino_SH8601(_bus, GFX_NOT_DEFINED /* RST via expander */,
                                  0 /* portrait */, AMOLED_W, AMOLED_H);

        if (!_gfx->begin(AMOLED_QSPI_HZ))
            return false;

        _gfx->fillScreen(RGB565_BLACK);
        ((Arduino_SH8601*)_gfx)->setBrightness(AMOLED_BRIGHTNESS);

        // Frame buffer for the game area (368*230*2 = 169 KB).
        // Internal RAM first (faster), PSRAM as a fallback. Doom's own
        // 6 MB zone lives in PSRAM, so keep that free if we can.
        const size_t bytes = (size_t)GAME_W * GAME_H * sizeof(uint16_t);
        _fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!_fb)
            _fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!_fb)
            return false;
        memset(_fb, 0, bytes);

        buildScaleTables();
        return true;
    }

    void beginFrame() override {}

    void writeRow(int x, int y, const uint16_t* rgb565, int count) override
    {
        // Native mode: library always sends x=0, 320 pixels, y in 0..199.
        if (x != 0 || count < SRC_W || y < 0 || y >= SRC_H) return;

        const int first = _rowFirst[y];
        const int n     = _rowCount[y];
        if (n == 0) return;

        uint16_t* dst = _fb + (size_t)first * GAME_W;
        for (int dx = 0; dx < GAME_W; dx++)
            dst[dx] = rgb565[_xLUT[dx]];

        // Rows that map to the same source row are just copies.
        for (int r = 1; r < n; r++)
            memcpy(dst + (size_t)r * GAME_W, dst, GAME_W * sizeof(uint16_t));
    }

    void endFrame() override
    {
        _gfx->draw16bitRGBBitmap(GAME_X, GAME_Y, _fb, GAME_W, GAME_H);
    }

    int width()  const override { return SRC_W; }
    int height() const override { return SRC_H; }

private:
    void buildScaleTables()
    {
        for (int dx = 0; dx < GAME_W; dx++)
            _xLUT[dx] = (uint16_t)((dx * SRC_W) / GAME_W);

        memset(_rowCount, 0, sizeof(_rowCount));
        for (int dy = 0; dy < GAME_H; dy++)
        {
            int sy = (dy * SRC_H) / GAME_H;
            if (_rowCount[sy] == 0) _rowFirst[sy] = (uint16_t)dy;
            _rowCount[sy]++;
        }
    }

    Arduino_DataBus* _bus = nullptr;
    Arduino_GFX*     _gfx = nullptr;
    uint16_t*        _fb  = nullptr;

    uint16_t _xLUT[GAME_W];
    uint16_t _rowFirst[SRC_H];
    uint8_t  _rowCount[SRC_H];
};
