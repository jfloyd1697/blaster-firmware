#ifndef BLASTER_CORE_TONE_GENERATOR_H
#define BLASTER_CORE_TONE_GENERATOR_H

#pragma once

#include <cstdint>

// Small, allocation-free PCM tone generator. A triangle wave avoids requiring
// a sine table or float operations inside an SD/I2S DMA refill callback.
class ToneGenerator {
public:
    ToneGenerator() = default;

    ToneGenerator(const std::uint32_t frequencyHz,
                  const std::uint32_t sampleRate,
                  const std::int16_t amplitude) {
        configure(frequencyHz, sampleRate, amplitude);
    }

    bool configure(const std::uint32_t frequencyHz,
                   const std::uint32_t sampleRate,
                   const std::int16_t amplitude) {
        if (sampleRate == 0 || frequencyHz == 0 || frequencyHz >= sampleRate / 2 ||
            amplitude <= 0 || amplitude > 16000) {
            m_step = 0;
            return false;
        }
        m_phase = 0;
        m_amplitude = amplitude;
        m_step = static_cast<std::uint32_t>((static_cast<std::uint64_t>(frequencyHz) << 32) / sampleRate);
        return m_step != 0;
    }

    [[nodiscard]] std::int16_t nextSample() {
        const std::uint32_t phase = m_phase >> 16;
        const std::uint32_t triangle = phase < 32768u ? phase : 65535u - phase;
        const std::int32_t sample =
            static_cast<std::int32_t>((triangle * 2u * m_amplitude) / 32767u) - m_amplitude;
        m_phase += m_step;
        return static_cast<std::int16_t>(sample);
    }

private:
    std::uint32_t m_step = 0;
    std::uint32_t m_phase = 0;
    std::int16_t m_amplitude = 0;
};

#endif // BLASTER_CORE_TONE_GENERATOR_H
