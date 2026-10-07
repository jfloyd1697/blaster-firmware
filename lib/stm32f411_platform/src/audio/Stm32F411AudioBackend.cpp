#include "platform/stm32f411/audio/Stm32F411AudioBackend.h"

#include <Arduino.h>

#include <algorithm>
#include <cstring>

#include "core/debug/IDebug.h"

namespace {
I2S_HandleTypeDef g_i2s3{};
DMA_HandleTypeDef g_i2s3Dma{};
Stm32F411AudioBackend* g_activeAudio = nullptr;

bool readExact(File& file, void* data, const std::size_t size) {
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

extern "C" void DMA1_Stream5_IRQHandler() {
    HAL_DMA_IRQHandler(&g_i2s3Dma);
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI3 && g_activeAudio != nullptr) {
        g_activeAudio->onDmaHalfComplete();
    }
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI3 && g_activeAudio != nullptr) {
        g_activeAudio->onDmaComplete();
    }
}

extern "C" void HAL_I2S_ErrorCallback(I2S_HandleTypeDef* hi2s) {
    if (hi2s->Instance == SPI3 && g_activeAudio != nullptr) {
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
    if (m_pins.ws != PA4 || m_pins.bclk != PC10 || m_pins.data != PC12) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: current HAL backend requires I2S3 PA4/PC10/PC12");
        }
        return false;
    }

    if (!initializeHardware()) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: I2S3/DMA initialization failed");
        }
        return false;
    }

    g_activeAudio = this;
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: I2S3 DMA output initialized");
    }
    return true;
}

void Stm32F411AudioBackend::update() {
    if (!m_active) {
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

    if (!configureI2s(m_format.sampleRate)) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: unsupported sample rate " + std::to_string(m_format.sampleRate));
        }
        stop();
        return;
    }

    m_dataBytesRead = 0;
    m_sourceExhausted = false;
    fillHalf(0);
    fillHalf(1);

    m_refillFirstHalf = false;
    m_refillSecondHalf = false;
    m_dmaError = false;
    m_finishPending = false;
    m_active = true;

    if (HAL_I2S_Transmit_DMA(
            &g_i2s3,
            reinterpret_cast<std::uint16_t*>(m_dmaBuffer.data()),
            static_cast<std::uint16_t>(m_dmaBuffer.size())) != HAL_OK) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: failed to start I2S DMA");
        }
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

void Stm32F411AudioBackend::stop() {
    if (g_i2s3.Instance != nullptr) {
        HAL_I2S_DMAStop(&g_i2s3);
    }

    if (m_file) {
        m_file.close();
    }

    m_currentFile.clear();
    m_loop = false;
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
    if (!SD.exists(path.c_str())) {
        if (m_debug != nullptr) {
            m_debug->error("Stm32F411AudioBackend: file does not exist: " + path);
        }
        return false;
    }

    m_file = SD.open(path.c_str(), FILE_READ);
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

    return m_file.seek(m_format.dataOffset);
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
        const std::uint32_t chunkDataOffset = m_file.position();

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
        if (!m_file.seek(nextChunk)) {
            return false;
        }
    }

    return haveFormat && haveData && (format.sampleRate == 22050u || format.sampleRate == 44100u);
}

bool Stm32F411AudioBackend::configureI2s(const std::uint32_t sampleRate) {
    HAL_I2S_DMAStop(&g_i2s3);
    g_i2s3.Init.AudioFreq = sampleRate;
    return HAL_I2S_Init(&g_i2s3) == HAL_OK;
}

bool Stm32F411AudioBackend::initializeHardware() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef clocks{};
    clocks.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    clocks.PLLI2S.PLLI2SN = 271;
    clocks.PLLI2S.PLLI2SR = 2;
    if (HAL_RCCEx_PeriphCLKConfig(&clocks) != HAL_OK) {
        return false;
    }

    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF6_SPI3;

    gpio.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &gpio);

    g_i2s3.Instance = SPI3;
    g_i2s3.Init.Mode = I2S_MODE_MASTER_TX;
    g_i2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    g_i2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    g_i2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    g_i2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    g_i2s3.Init.CPOL = I2S_CPOL_LOW;
    g_i2s3.Init.ClockSource = I2S_CLOCK_PLL;
    g_i2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    g_i2s3Dma.Instance = DMA1_Stream5;
    g_i2s3Dma.Init.Channel = DMA_CHANNEL_0;
    g_i2s3Dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_i2s3Dma.Init.PeriphInc = DMA_PINC_DISABLE;
    g_i2s3Dma.Init.MemInc = DMA_MINC_ENABLE;
    g_i2s3Dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_i2s3Dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    g_i2s3Dma.Init.Mode = DMA_CIRCULAR;
    g_i2s3Dma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    g_i2s3Dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&g_i2s3Dma) != HAL_OK) {
        return false;
    }

    __HAL_LINKDMA(&g_i2s3, hdmatx, g_i2s3Dma);
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    return HAL_I2S_Init(&g_i2s3) == HAL_OK;
}

bool Stm32F411AudioBackend::fillHalf(const std::size_t halfIndex) {
    auto* destination = m_dmaBuffer.data() + halfIndex * kHalfWordCount;
    const std::size_t framesRead = readPcmFrames(destination, kFramesPerHalf);
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
    if (!m_file.seek(m_format.dataOffset)) {
        return false;
    }
    m_dataBytesRead = 0;
    m_sourceExhausted = false;
    m_halvesUntilStop = 0;
    return true;
}

void Stm32F411AudioBackend::finishPlayback() {
    HAL_I2S_DMAStop(&g_i2s3);
    if (m_file) {
        m_file.close();
    }
    m_active = false;
    m_currentFile.clear();
    if (m_debug != nullptr) {
        m_debug->log("Stm32F411AudioBackend: playback finished");
    }
}
