#ifndef BLASTER_STM32F411_AUDIO_BACKEND_H
#define BLASTER_STM32F411_AUDIO_BACKEND_H

#pragma once

#include <SdFat.h>
#include <stm32f4xx_hal.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include "core/audio/IAudioBackend.h"
#include "core/audio/ToneGenerator.h"
#include "platform/stm32f411/board/Stm32F411BoardConfig.h"

struct IDebug;

class Stm32F411AudioBackend : public IAudioBackend {
public:
    explicit Stm32F411AudioBackend(const Stm32F411I2sPins& pins, IDebug* debug = nullptr);
    ~Stm32F411AudioBackend() override;

    bool begin() override;
    void update() override;
    void playSound(const std::string& file, bool loop, bool blocking) override;
    bool playTone(std::uint32_t frequencyHz, std::uint32_t durationMs) override;
    bool prepareSounds(const std::vector<std::string>& paths) override;
    void releasePreparedSounds() override;
    void stop() override;

    void onDmaHalfComplete();
    void onDmaComplete();
    void onDmaError();
    [[nodiscard]] std::uint32_t refillDeadlineMisses() const { return m_refillDeadlineMisses; }

private:
    struct WavFormat {
        std::uint16_t channels = 0;
        std::uint32_t sampleRate = 0;
        std::uint16_t bitsPerSample = 0;
        std::uint32_t dataOffset = 0;
        std::uint32_t dataSize = 0;
    };

    struct PreparedClip {
        std::string path;
        File32 file;
        WavFormat format;
    };

    static constexpr std::size_t kFramesPerHalf = 1024;
    static constexpr std::size_t kHalfWordCount = kFramesPerHalf * 2;
    static constexpr std::size_t kBufferWordCount = kHalfWordCount * 2;

    [[nodiscard]] static std::string normalizePath(const std::string& path);
    bool openWav(const std::string& path);
    bool parseWavHeader(File32& file, WavFormat& format);
    bool checkWavFormat(const WavFormat& format, const std::string& path) const;
    bool initializeHardware();
    [[nodiscard]] bool isI2sConfigured() const;
    bool ensureI2sReady();
    void startSoundNow(const std::string& file, bool loop);
    bool startBufferedPlayback();
    bool fillHalf(std::size_t halfIndex);
    std::size_t readPcmFrames(std::int16_t* destination, std::size_t frameCount);
    bool rewindAudioData();
    void finishPlayback();

    Stm32F411I2sPins m_pins;
    IDebug* m_debug = nullptr;
    File32 m_file; // For standalone diagnostics and non-prepared paths.
    File32* m_playbackFile = nullptr;
    std::vector<std::unique_ptr<PreparedClip>> m_preparedClips;
    WavFormat m_format;
    std::string m_currentFile;
    struct PendingPlayback {
        std::string path;
        bool loop = false;
    };
    std::unique_ptr<PendingPlayback> m_pendingPlayback;
    std::array<std::int16_t, kBufferWordCount> m_dmaBuffer{};
    std::array<std::int16_t, kFramesPerHalf> m_monoScratch{};
    volatile std::uint32_t m_refillDeadlineMisses = 0;
    volatile bool m_refillFirstHalf = false;
    volatile bool m_refillSecondHalf = false;
    volatile bool m_dmaError = false;
    volatile bool m_finishPending = false;
    volatile std::uint8_t m_halvesUntilStop = 0;
    ToneGenerator m_tone;
    std::uint32_t m_toneFramesRemaining = 0;
    bool m_toneActive = false;
    bool m_loop = false;
    bool m_active = false;
    bool m_sourceExhausted = false;
    std::uint32_t m_dataBytesRead = 0;
};

#endif // BLASTER_STM32F411_AUDIO_BACKEND_H
