// Standalone STM32F411 -> MAX98357A I2S test.
// No DMA, SD card, application services, or physical buttons required.
// Flash with: pio run -e stm32f411_blackpill_i2s_example -t upload
// Pins: PB12 = WS/LRC, PB13 = BCLK, PB15 = DIN.

#include <Arduino.h>
#include <stm32f4xx_hal.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {
constexpr std::size_t kWaveSize = 256;
constexpr std::size_t kFramesPerChunk = 64;
constexpr std::uint32_t kSampleRate = 44100;
constexpr std::uint32_t kToneHz = 440;
constexpr std::uint32_t kPhaseStep = static_cast<std::uint32_t>(
    (static_cast<double>(kToneHz) * 4294967296.0) / kSampleRate);

I2S_HandleTypeDef i2s{};
std::int16_t wave[kWaveSize]{};
std::int16_t frames[kFramesPerChunk * 2]{};
std::uint32_t phase = 0;
bool ready = false;

void printStatus(const char* stage) {
    Serial.print(stage);
    Serial.print(" HAL state=");
    Serial.print(static_cast<unsigned>(i2s.State));
    Serial.print(" error=");
    Serial.print(static_cast<unsigned long>(i2s.ErrorCode));
    Serial.print(" I2SCFGR=0x");
    Serial.print(static_cast<unsigned long>(SPI2->I2SCFGR), HEX);
    Serial.print(" I2SPR=0x");
    Serial.println(static_cast<unsigned long>(SPI2->I2SPR), HEX);
}

bool initializeI2s() {
    // Use the clock parameters already chosen by the STM32duino board startup.
    const std::uint32_t pllInputDivider = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
    if (pllInputDivider < 2u || pllInputDivider > 63u) {
        Serial.println("ERROR: invalid system PLL input divider");
        return false;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2S_CONFIG(RCC_I2SCLKSOURCE_PLLI2S);
    __HAL_RCC_SPI2_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef clocks{};
    clocks.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    clocks.PLLI2S.PLLI2SM = pllInputDivider; // Required on STM32F411!
    clocks.PLLI2S.PLLI2SN = 271;
    clocks.PLLI2S.PLLI2SR = 2;
    if (HAL_RCCEx_PeriphCLKConfig(&clocks) != HAL_OK) {
        Serial.println("ERROR: PLLI2S configuration failed");
        return false;
    }

    Serial.print("I2S kernel clock: ");
    Serial.print(static_cast<unsigned long>(HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2S)));
    Serial.println(" Hz");

    GPIO_InitTypeDef gpio{};
    gpio.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &gpio);

    i2s.Instance = SPI2;
    i2s.Init.Mode = I2S_MODE_MASTER_TX;
    i2s.Init.Standard = I2S_STANDARD_PHILIPS;
    i2s.Init.DataFormat = I2S_DATAFORMAT_16B;
    i2s.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    i2s.Init.AudioFreq = I2S_AUDIOFREQ_44K;
    i2s.Init.CPOL = I2S_CPOL_LOW;
    i2s.Init.ClockSource = I2S_CLOCK_PLL;
    i2s.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    if (HAL_I2S_Init(&i2s) != HAL_OK) {
        printStatus("HAL_I2S_Init FAILED");
        return false;
    }
    printStatus("HAL_I2S_Init OK");
    return true;
}
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Standalone STM32F411 I2S output test (440 Hz, 44.1 kHz stereo)");

    constexpr double twoPi = 6.283185307179586;
    for (std::size_t i = 0; i < kWaveSize; ++i) {
        wave[i] = static_cast<std::int16_t>(
            2400.0 * std::sin(twoPi * static_cast<double>(i) / kWaveSize));
    }
    ready = initializeI2s();
}

void loop() {
    if (!ready) {
        delay(1000);
        return;
    }

    for (std::size_t index = 0; index < kFramesPerChunk; ++index) {
        const std::int16_t sample = wave[phase >> 24u];
        phase += kPhaseStep;
        frames[index * 2] = sample;
        frames[index * 2 + 1] = sample;
    }

    if (HAL_I2S_Transmit(
            &i2s,
            reinterpret_cast<std::uint16_t*>(frames),
            static_cast<std::uint16_t>(kFramesPerChunk * 2),
            1000u) != HAL_OK) {
        printStatus("HAL_I2S_Transmit FAILED");
        ready = false;
    }
}
