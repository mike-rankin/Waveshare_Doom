#pragma once

// IDoomPCMOutput for the onboard ES8311 codec.
//
// ESP-DOOM's mixer task (core 0) calls begin(11025) once, then streams
// MONO 16-bit blocks into write(). We run I2S at that same rate with
// MCLK = 256 x fs (2.8224 MHz, which is in the ES8311 clock table),
// duplicate each sample to L+R, and enable the speaker amplifier.

#include <Arduino.h>
#include <ESP_I2S.h>
#include <IDoomPCMOutput.h>
#include "hw_conf.h"
#include "es8311.h"

class Es8311Output : public IDoomPCMOutput
{
public:
    bool begin(uint32_t sampleRate) override
    {
        _i2s.setPins(AUDIO_PIN_BCLK, AUDIO_PIN_WS, AUDIO_PIN_DOUT, -1 /* no mic */, AUDIO_PIN_MCLK);
        if (!_i2s.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO))
        {
            Serial.println("[audio] I2S begin failed");
            return false;
        }

        if (!codecInit(sampleRate))
        {
            Serial.println("[audio] ES8311 init failed (check I2C)");
            _i2s.end();
            return false;
        }

        pinMode(AUDIO_PIN_PA, OUTPUT);
        digitalWrite(AUDIO_PIN_PA, HIGH);   // speaker amp on
        Serial.printf("[audio] ES8311 running at %lu Hz\n", (unsigned long)sampleRate);
        return true;
    }

    void write(const int16_t* samples, size_t count) override
    {
        while (count)
        {
            const size_t n = (count > CHUNK) ? CHUNK : count;
            for (size_t i = 0; i < n; i++)
            {
                _stereo[i * 2]     = samples[i];
                _stereo[i * 2 + 1] = samples[i];
            }
            _i2s.write((const uint8_t*)_stereo, n * 2 * sizeof(int16_t));   // blocks = pacing
            samples += n;
            count   -= n;
        }
    }

    void end() override
    {
        digitalWrite(AUDIO_PIN_PA, LOW);
        if (_codec) { es8311_voice_mute(_codec, true); es8311_delete(_codec); _codec = nullptr; }
        _i2s.end();
    }

private:
    static constexpr size_t CHUNK = 256;   // ESP-DOOM mixes 256 frames per block

    bool codecInit(uint32_t rate)
    {
        _codec = es8311_create(0 /* I2C port used by Wire */, ES8311_ADDRESS_0);
        if (!_codec) return false;

        const es8311_clock_config_t clk = {
            .mclk_inverted      = false,
            .sclk_inverted      = false,
            .mclk_from_mclk_pin = true,
            .mclk_frequency     = (int)(rate * 256),
            .sample_frequency   = (int)rate,
        };

        if (es8311_init(_codec, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK) return false;
        if (es8311_sample_frequency_config(_codec, clk.mclk_frequency, clk.sample_frequency) != ESP_OK) return false;
        if (es8311_voice_volume_set(_codec, AUDIO_VOLUME, NULL) != ESP_OK) return false;
        es8311_voice_mute(_codec, false);
        return true;
    }

    I2SClass         _i2s;
    es8311_handle_t  _codec = nullptr;
    int16_t          _stereo[CHUNK * 2];
};
