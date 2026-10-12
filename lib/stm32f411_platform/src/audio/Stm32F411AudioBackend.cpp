#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"

#include <Arduino.h>

#include <algorithm>
#include <cstring>

#include "core/debug/IDebug.h"
#include "platform/stm32f411/storage/Stm32F411Sd.h"

namespace {
I2S_HandleTypeDef g_i2s2{};
DMA_HandleTypeDef g_i2s2Dma{};
Stm32F411AudioBackend* g_activeAudio = nullptr;

bool readExact(File32& file, void* data, const std::size_t size) {
    return file.read(static_cast<std::uint8_t*>(data), size) == static_cast<int>(size);
}

std::uint16_t readLe16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0]) |
           static_cast<std::uint16_t>(data[1]) << 8u;
}

std::uint32_t readLe32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) |
           static_cast<std::uint32_t>(data[1]) << 8u |
           static_cast<std::uint32_t>(data[2]) << 16u |
           static_cast<std::uint32_t>(data[3]) << 24u;
}
}

extern "C" void DMA1_Stream4_IRQHandler() {
    HAL_DMA_IRQHandler(&g_i2s2Dma);
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI2 && g_activeAudio != nullptr) {
        g_activeAudio->onDmaHalfComplete();
    }
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI2 && g_activeAudio != nullptr) {
        g_activeAudio->onDmaComplete();
    }
}

extern "C" void HAL_I2S_ErrorCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI2 && g_activeAudio != nullptr) {
        g_activeAudio->onDmaError();
    }
}

Stm32F411AudioBackend::Stm32F411AudioBackend(const Stm32F411I2sPins& pins, IDebug* debug)
    : m_pins(pins),
      m_debug(debug) {
}

Stm32F411AudioBackend::~Stm32F411AudioBackend() {
    stop();
    if (g_activeAudio == this) {
        g_activeAudio = nullptr;
    }
}

bool Stm32F411AudioBackend::begin() {
    if (m_pins.ws != PB12 || m_pins.bclk != PB13 || m_pins.data != PB15) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: current HAL backend requires I2S2 PB12/PB13/PB15");
        }
        return false;
    }

    if (!initializeHardware()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: I2S2/DMA initialization failed "
                           "(HAL state=" + std::to_string(static_cast<unsigned int>(g_i2s2.State)) +
                           ", HAL error=" + std::to_string(g_i2s2.ErrorCode) + ")");
        }
        return false;
    }

    g_activeAudio = this;
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: I2S2 DMA output initialized");
    }
    return true;
}

void Stm32F411AudioBackend::update() {
    if (!m_active) {
        return;
    }

    if (!isI2sConfigured()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: SPI2 lost I2S mode DURING playback; "
                           "stopping audio (I2SCFGR=" + std::to_string(SPI2->I2SCFGR) + ")");
        }
        stop();
        return;
    }

    if (m_dmaError) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: DMA playback error");
        }
        stop();
        return;
    }

    if (m_finishPending) {
        finishPlayback();
        return;
    }

    if (m_refillFirstHalf) {
        m_refillFirstHalf = false;
        fillHalf(0);
    }

    if (m_refillSecondHalf) {
        m_refillSecondHalf = false;
        fillHalf(1);
    }
}

void Stm32F411AudioBackend::playSound(const std::string& file, const bool loop, const bool blocking) {
    stop();

    m_currentFile = normalizePath(file);
    m_loop = loop;
    if (!openWav(m_currentFile)) {
        stop();
        return;
    }

    // I2S2 was initialized at 44.1 kHz by begin(). Do not call HAL_I2S_Init
    // again between clips: it can fail while the peripheral/DMA is active and
    // was being incorrectly reported as an unsupported 44100 Hz file.
    if (!ensureI2sReady()) {
        stop();
        return;
    }

    m_toneActive = false;
    if (!startBufferedPlayback()) {
        stop();
        return;
    }

    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: playing " + m_currentFile);
    }

    if (blocking && !loop) {
        while (m_active) {
            update();
            yield();
        }
    }
}

bool Stm32F411AudioBackend::playTone(const std::uint32_t frequencyHz,
                                      const std::uint32_t durationMs) {
    stop();
    if (frequencyHz < 100u || frequencyHz > 10000u || durationMs == 0u || durationMs > 5000u ||
        !m_tone.configure(frequencyHz, 44100u, 6000)) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: invalid diagnostic tone parameters");
        }
        return false;
    }
    if (!ensureI2sReady()) {
        return false;
    }

    m_toneActive = true;
    m_toneFramesRemaining = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(44100u) * durationMs) / 1000u);
    if (!startBufferedPlayback()) {
        stop();
        return false;
    }
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: diagnostic tone " +
                     std::to_string(frequencyHz) + " Hz for " +
                     std::to_string(durationMs) + " ms");
    }
    return true;
}

bool Stm32F411AudioBackend::startBufferedPlayback() {
    m_dataBytesRead = 0;
    m_sourceExhausted = false;
    m_halvesUntilStop = 0;
    m_refillFirstHalf = false;
    m_refillSecondHalf = false;
    m_dmaError = false;
    m_finishPending = false;
    fillHalf(0);
    fillHalf(1);
    m_active = true;

    if (HAL_I2S_Transmit_DMA(
            &g_i2s2,
            reinterpret_cast<std::uint16_t*>(m_dmaBuffer.data()),
            static_cast<std::uint16_t>(m_dmaBuffer.size())) != HAL_OK) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: failed to start I2S DMA");
        }
        return false;
    }
    if (!isI2sConfigured()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: SPI2 not in I2S mode after DMA start");
        }
        HAL_I2S_DMAStop(&g_i2s2);
        return false;
    }
    return true;
}

void Stm32F411AudioBackend::stop() {
    if (m_active && g_i2s2.Instance != nullptr) {
        HAL_I2S_DMAStop(&g_i2s2);
    }

    if (m_file) {
        m_file.close();
    }

    m_currentFile.clear();
    m_loop = false;
    m_toneActive = false;
    m_toneFramesRemaining = 0;
    m_active = false;
    m_sourceExhausted = false;
    m_refillFirstHalf = false;
    m_refillSecondHalf = false;
    m_dmaError = false;
    m_finishPending = false;
    m_halvesUntilStop = 0;
    m_dataBytesRead = 0;
}

void Stm32F411AudioBackend::onDmaHalfComplete() {
    if (m_sourceExhausted && m_halvesUntilStop > 0) {
        --m_halvesUntilStop;
        if (m_halvesUntilStop == 0) {
            m_finishPending = true;
            return;
        }
    }
    m_refillFirstHalf = true;
}

void Stm32F411AudioBackend::onDmaComplete() {
    if (m_sourceExhausted && m_halvesUntilStop > 0) {
        --m_halvesUntilStop;
        if (m_halvesUntilStop == 0) {
            m_finishPending = true;
            return;
        }
    }
    m_refillSecondHalf = true;
}

void Stm32F411AudioBackend::onDmaError() {
    m_dmaError = true;
}

std::string Stm32F411AudioBackend::normalizePath(const std::string& path) {
    std::string normalized = path;
    for (char& ch : normalized) {
        if (ch == '\\') {
            ch = '/';
        }
    }
    return normalized;
}

bool Stm32F411AudioBackend::openWav(const std::string& path) {
    m_file = g_stm32Sd.open(path.c_str(), O_RDONLY);
    if (!m_file) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: failed to open " + path);
        }
        return false;
    }

    if (!parseWavHeader(m_format)) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: unsupported WAV file " + path);
        }
        m_file.close();
        return false;
    }

    // The SD exporter normalizes authored sounds to 44.1 kHz PCM. The
    // STM32 I2S clock is configured for that rate; do not attempt to
    // retune the peripheral for arbitrary source rates during playback.
    if (m_format.sampleRate != 44100u) {
        if (m_debug != nullptr) {
            m_debug->error(
                "Stm32F411AudioBackend: expected 44100 Hz PCM WAV, got " +
                std::to_string(m_format.sampleRate) + " Hz: " + path +
                ". Re-export the audio using weapon-fsm-export-sd.");
        }
        m_file.close();
        return false;
    }

    return m_file.seekSet(m_format.dataOffset);
}

bool Stm32F411AudioBackend::parseWavHeader(WavFormat& format) {
    std::uint8_t header[12]{};
    if (!readExact(m_file, header, sizeof(header)) ||
        std::memcmp(header, "RIFF", 4) != 0 ||
        std::memcmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }

    bool haveFormat = false;
    bool haveData = false;
    while (m_file.available() && (!haveFormat || !haveData)) {
        std::uint8_t chunkHeader[8]{};
        if (!readExact(m_file, chunkHeader, sizeof(chunkHeader))) {
            return false;
        }

        const std::uint32_t chunkSize = readLe32(chunkHeader + 4);
        const std::uint32_t chunkDataOffset = static_cast<std::uint32_t>(m_file.curPosition());

        if (std::memcmp(chunkHeader, "fmt ", 4) == 0) {
            if (chunkSize < 16) {
                return false;
            }
            std::uint8_t fmt[16]{};
            if (!readExact(m_file, fmt, sizeof(fmt))) {
                return false;
            }
            const std::uint16_t encoding = readLe16(fmt);
            format.channels = readLe16(fmt + 2);
            format.sampleRate = readLe32(fmt + 4);
            format.bitsPerSample = readLe16(fmt + 14);
            if (encoding != 1 || (format.channels != 1 && format.channels != 2) || format.bitsPerSample != 16) {
                return false;
            }
            haveFormat = true;
        } else if (std::memcmp(chunkHeader, "data", 4) == 0) {
            format.dataOffset = chunkDataOffset;
            format.dataSize = chunkSize;
            haveData = true;
        }

        const std::uint32_t nextChunk = chunkDataOffset + chunkSize + (chunkSize & 1u);
        if (!m_file.seekSet(nextChunk)) {
            return false;
        }
    }

    return haveFormat && haveData && format.sampleRate != 0u;
}

bool Stm32F411AudioBackend::initializeHardware() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // STM32F411: the I2S PLL has its own input prescaler (PLLI2SM).
    // Use the already configured system PLL input divider so the I2S PLL
    // sees the same reference input for HSI- and HSE-based boards.
    const std::uint32_t pllInputDivider = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
    if (pllInputDivider < 2u || pllInputDivider > 63u) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: invalid PLL input divider for I2S");
        }
        return false;
    }

    __HAL_RCC_I2S_CONFIG(RCC_I2SCLKSOURCE_PLLI2S);
    __HAL_RCC_SPI2_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef clocks{};
    clocks.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    clocks.PLLI2S.PLLI2SM = pllInputDivider;
    clocks.PLLI2S.PLLI2SN = 271;
    clocks.PLLI2S.PLLI2SR = 2;
    if (HAL_RCCEx_PeriphCLKConfig(&clocks) != HAL_OK) {
        if (m_debug) m_debug->error("Stm32F411AudioBackend: PLLI2S clock configuration failed");
        return false;
    }

    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI2;

    gpio.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &gpio);

    g_i2s2.Instance = SPI2;
    g_i2s2.Init.Mode = I2S_MODE_MASTER_TX;
    g_i2s2.Init.Standard = I2S_STANDARD_PHILIPS;
    g_i2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
    g_i2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    g_i2s2.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    g_i2s2.Init.CPOL = I2S_CPOL_LOW;
    g_i2s2.Init.ClockSource = I2S_CLOCK_PLL;
    g_i2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    g_i2s2Dma.Instance = DMA1_Stream4;
    g_i2s2Dma.Init.Channel = DMA_CHANNEL_0;
    g_i2s2Dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_i2s2Dma.Init.PeriphInc = DMA_PINC_DISABLE;
    g_i2s2Dma.Init.MemInc = DMA_MINC_ENABLE;
    g_i2s2Dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_i2s2Dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    g_i2s2Dma.Init.Mode = DMA_CIRCULAR;
    g_i2s2Dma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    g_i2s2Dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&g_i2s2Dma) != HAL_OK) {
        if (m_debug) m_debug->error("Stm32F411AudioBackend: DMA1 stream 4 init failed (error " +
                                   std::to_string(g_i2s2Dma.ErrorCode) + ")");
        return false;
    }

    __HAL_LINKDMA(&g_i2s2, hdmatx, g_i2s2Dma);
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    const HAL_StatusTypeDef i2sStatus = HAL_I2S_Init(&g_i2s2);
    if (i2sStatus != HAL_OK && m_debug) {
        m_debug->error("Stm32F411AudioBackend: HAL_I2S_Init failed (HAL " +
                       std::to_string(static_cast<int>(i2sStatus)) + ", error " +
                       std::to_string(g_i2s2.ErrorCode) + ")");
    }
    if (i2sStatus != HAL_OK) {
        return false;
    }
    if (g_i2s2.State != HAL_I2S_STATE_READY) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: HAL_I2S_Init returned OK but handle state is " +
                           std::to_string(static_cast<unsigned int>(g_i2s2.State)));
        }
        return false;
    }
    if (!isI2sConfigured()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: HAL_I2S_Init returned OK but SPI2 "
                           "is not in I2S master TX mode (I2SCFGR=" +
                           std::to_string(SPI2->I2SCFGR) + ", I2SPR=" +
                           std::to_string(SPI2->I2SPR) + ")");
        }
        return false;
    }
    return true;
}

bool Stm32F411AudioBackend::isI2sConfigured() const {
    const std::uint32_t configuration = SPI2->I2SCFGR;
    const bool i2sSelected = (configuration & SPI_I2SCFGR_I2SMOD) != 0u;
    const bool transmitterSelected = (configuration & SPI_I2SCFGR_I2SCFG) == I2S_MODE_MASTER_TX;
    const bool prescalerConfigured = (SPI2->I2SPR & SPI_I2SPR_I2SDIV) > 2u;
    return i2sSelected && transmitterSelected && prescalerConfigured;
}

bool Stm32F411AudioBackend::ensureI2sReady() {
    // The platform keeps the audio service available for diagnostics even if
    // begin() failed. In that case the HAL handle is RESET/ERROR, not READY.
    // Retry the *complete* peripheral initialization instead of treating the
    // uninitialized handle as an unrecoverable playback error.
    if (g_i2s2.State == HAL_I2S_STATE_READY && isI2sConfigured()) {
        g_activeAudio = this;
        return true;
    }

    // Never change SPI2 or its DMA configuration while a transfer is active.
    // This also catches a stale BUSY_TX HAL handle that needs investigation.
    if ((DMA1_Stream4->CR & DMA_SxCR_EN) != 0u) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: cannot initialize I2S while DMA1 Stream4 is enabled "
                           "(HAL state=" + std::to_string(static_cast<unsigned int>(g_i2s2.State)) + ")");
        }
        return false;
    }

    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: retrying full I2S2 initialization "
                     "(HAL state=" + std::to_string(static_cast<unsigned int>(g_i2s2.State)) +
                     ", I2SCFGR=" + std::to_string(SPI2->I2SCFGR) +
                     ", I2SPR=" + std::to_string(SPI2->I2SPR) + ")");
    }

    if (!initializeHardware()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: I2S2 recovery failed "
                           "(HAL state=" + std::to_string(static_cast<unsigned int>(g_i2s2.State)) +
                           ", HAL error=" + std::to_string(g_i2s2.ErrorCode) +
                           ", I2SCFGR=" + std::to_string(SPI2->I2SCFGR) +
                           ", I2SPR=" + std::to_string(SPI2->I2SPR) + ")");
        }
        return false;
    }

    if (g_i2s2.State != HAL_I2S_STATE_READY || !isI2sConfigured()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: I2S2 initialization left an unusable HAL handle "
                           "(state=" + std::to_string(static_cast<unsigned int>(g_i2s2.State)) + ")");
        }
        return false;
    }

    g_activeAudio = this;
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: I2S2 recovery completed");
    }
    return true;
}

bool Stm32F411AudioBackend::fillHalf(const std::size_t halfIndex) {
    auto* destination = m_dmaBuffer.data() + halfIndex * kHalfWordCount;
    std::size_t framesRead = 0;
    if (m_toneActive && !m_sourceExhausted) {
        framesRead = std::min(kFramesPerHalf, static_cast<std::size_t>(m_toneFramesRemaining));
        for (std::size_t index = 0; index < framesRead; ++index) {
            const std::int16_t sample = m_tone.nextSample();
            destination[index * 2] = sample;
            destination[index * 2 + 1] = sample;
        }
        m_toneFramesRemaining -= static_cast<std::uint32_t>(framesRead);
    } else if (!m_toneActive) {
        framesRead = readPcmFrames(destination, kFramesPerHalf);
    }
    if (framesRead < kFramesPerHalf) {
        std::fill(
            destination + framesRead * 2,
            destination + kHalfWordCount,
            static_cast<std::int16_t>(0));
        if (!m_sourceExhausted) {
            m_halvesUntilStop = 2;
        }
        m_sourceExhausted = true;
    }
    return framesRead > 0;
}

std::size_t Stm32F411AudioBackend::readPcmFrames(std::int16_t* destination, const std::size_t frameCount) {
    if (!m_file || m_sourceExhausted) {
        return 0;
    }

    std::size_t framesProduced = 0;
    while (framesProduced < frameCount) {
        const std::uint32_t bytesRemaining = m_format.dataSize - m_dataBytesRead;
        if (bytesRemaining == 0) {
            if (m_loop && rewindAudioData()) {
                continue;
            }
            break;
        }

        const std::size_t sourceBytesPerFrame = static_cast<std::size_t>(m_format.channels) * sizeof(std::int16_t);
        const std::size_t framesAvailable = bytesRemaining / sourceBytesPerFrame;
        const std::size_t framesToRead = std::min(frameCount - framesProduced, framesAvailable);
        if (framesToRead == 0) {
            break;
        }

        if (m_format.channels == 2) {
            const std::size_t bytes = framesToRead * 2 * sizeof(std::int16_t);
            const int bytesRead = m_file.read(
                reinterpret_cast<std::uint8_t*>(destination + framesProduced * 2), bytes);
            if (bytesRead <= 0) {
                break;
            }
            const std::size_t actualFrames = static_cast<std::size_t>(bytesRead) / (2 * sizeof(std::int16_t));
            m_dataBytesRead += static_cast<std::uint32_t>(actualFrames * 2 * sizeof(std::int16_t));
            framesProduced += actualFrames;
            if (actualFrames < framesToRead) {
                break;
            }
        } else {
            const std::size_t bytes = framesToRead * sizeof(std::int16_t);
            const int bytesRead = m_file.read(reinterpret_cast<std::uint8_t*>(m_monoScratch.data()), bytes);
            if (bytesRead <= 0) {
                break;
            }
            const std::size_t actualFrames = static_cast<std::size_t>(bytesRead) / sizeof(std::int16_t);
            for (std::size_t index = 0; index < actualFrames; ++index) {
                const std::int16_t sample = m_monoScratch[index];
                destination[(framesProduced + index) * 2] = sample;
                destination[(framesProduced + index) * 2 + 1] = sample;
            }
            m_dataBytesRead += static_cast<std::uint32_t>(actualFrames * sizeof(std::int16_t));
            framesProduced += actualFrames;
            if (actualFrames < framesToRead) {
                break;
            }
        }
    }

    return framesProduced;
}

bool Stm32F411AudioBackend::rewindAudioData() {
    if (!m_file.seekSet(m_format.dataOffset)) {
        return false;
    }
    m_dataBytesRead = 0;
    m_sourceExhausted = false;
    m_halvesUntilStop = 0;
    return true;
}

void Stm32F411AudioBackend::finishPlayback() {
    HAL_I2S_DMAStop(&g_i2s2);
    if (m_file) {
        m_file.close();
    }
    m_active = false;
    m_toneActive = false;
    m_toneFramesRemaining = 0;
    m_currentFile.clear();
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: playback finished");
    }
}
