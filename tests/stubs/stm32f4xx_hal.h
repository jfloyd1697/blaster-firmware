#pragma once
#include <cstddef>
#include <cstdint>

enum HAL_StatusTypeDef { HAL_OK = 0, HAL_ERROR = 1 };
constexpr int HAL_I2S_STATE_READY = 1;
constexpr int HAL_I2S_STATE_BUSY_TX = 2;
struct DMA_HandleTypeDef;
struct I2S_InitTypeDef {
    int Mode{}, Standard{}, DataFormat{}, MCLKOutput{}, AudioFreq{}, CPOL{}, ClockSource{}, FullDuplexMode{};
};
struct I2S_HandleTypeDef {
    void* Instance{};
    int State{};
    I2S_InitTypeDef Init{};
    DMA_HandleTypeDef* hdmatx{};
    std::uint32_t ErrorCode = 0;
};
struct DMA_InitTypeDef {
    int Channel{}, Direction{}, PeriphInc{}, MemInc{}, PeriphDataAlignment{}, MemDataAlignment{};
    int Mode{}, Priority{}, FIFOMode{};
};
struct DMA_HandleTypeDef { void* Instance{}; DMA_InitTypeDef Init{}; std::uint32_t ErrorCode = 0; };
struct RCC_PeriphCLKInitTypeDef {
    int PeriphClockSelection{}, PLLI2SN{}, PLLI2SR{};
    struct { int PLLI2SM{}, PLLI2SN{}, PLLI2SR{}; } PLLI2S{};
};
struct GPIO_InitTypeDef { int Mode{}, Pull{}, Speed{}, Alternate{}, Pin{}; };
struct StubSpiRegisterBlock {
    std::uint32_t CR2 = 0;
    std::uint32_t SR = 0;
    std::uint32_t I2SCFGR = 0;
    std::uint32_t I2SPR = 2;
};
inline StubSpiRegisterBlock stubSpi2;
inline int testI2sInitCount = 0;
inline int testConfiguredPllI2sM = 0;
inline int testI2sSelectedClockSource = -1;
inline bool testI2sInitFailsToConfigure = false;
inline bool testI2sInitReturnsError = false;
#define SPI_I2SCFGR_I2SMOD (1u << 11u)
#define SPI_I2SCFGR_I2SCFG (3u << 8u)
#define SPI_I2SPR_I2SDIV 0xffu
struct StubDmaStream { std::uint32_t CR = 0; };
inline StubDmaStream stubDma;
#define DMA_SxCR_EN 1u
inline int stubGpio;
#define SPI2 (&stubSpi2)
#define DMA1_Stream4 (&stubDma)
#define GPIOB (&stubGpio)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SPI2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DMA1_CLK_ENABLE() ((void)0)
#define __HAL_LINKDMA(handle, field, dma) ((handle)->field = &(dma))
#define RCC_PERIPHCLK_I2S 1
#define RCC_I2SCLKSOURCE_PLLI2S 0
#define __HAL_RCC_I2S_CONFIG(source) (testI2sSelectedClockSource = (source))
#define GPIO_MODE_AF_PP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_VERY_HIGH 2
#define GPIO_AF5_SPI2 5
#define GPIO_PIN_12 (1<<12)
#define GPIO_PIN_13 (1<<13)
#define GPIO_PIN_15 (1<<15)
#define I2S_MODE_MASTER_TX (2u << 8u)
#define I2S_STANDARD_PHILIPS 1
#define I2S_DATAFORMAT_16B 1
#define I2S_MCLKOUTPUT_DISABLE 0
#define I2S_AUDIOFREQ_44K 44100
#define I2S_CPOL_LOW 0
#define I2S_CLOCK_PLL 1
#define I2S_FULLDUPLEXMODE_DISABLE 0
#define DMA_CHANNEL_0 0
#define DMA_MEMORY_TO_PERIPH 1
#define DMA_PINC_DISABLE 0
#define DMA_MINC_ENABLE 1
#define DMA_PDATAALIGN_HALFWORD 2
#define DMA_MDATAALIGN_HALFWORD 2
#define DMA_CIRCULAR 1
#define DMA_PRIORITY_VERY_HIGH 3
#define DMA_FIFOMODE_DISABLE 0
#define DMA1_Stream4_IRQn 1
inline std::uint16_t* testDmaBuffer = nullptr;
inline std::uint16_t testDmaLength = 0;
inline int testTransmitCount = 0;
inline int testStopCount = 0;
inline HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(const RCC_PeriphCLKInitTypeDef* config) {
    testConfiguredPllI2sM = config->PLLI2S.PLLI2SM;
    return HAL_OK;
}
inline void HAL_GPIO_Init(void*, GPIO_InitTypeDef*) {}
inline HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef*) { stubDma.CR = 0; return HAL_OK; }
inline void HAL_NVIC_SetPriority(int, int, int) {}
inline void HAL_NVIC_EnableIRQ(int) {}
inline HAL_StatusTypeDef HAL_I2S_Init(I2S_HandleTypeDef* handle) {
    ++testI2sInitCount;
    if (testConfiguredPllI2sM == 0 || testI2sSelectedClockSource != RCC_I2SCLKSOURCE_PLLI2S) {
        handle->ErrorCode = 16;
        return HAL_ERROR;
    }
    if (testI2sInitReturnsError) return HAL_ERROR;
    handle->State = HAL_I2S_STATE_READY;
    if (testI2sInitFailsToConfigure) return HAL_OK;
    stubSpi2.I2SCFGR = SPI_I2SCFGR_I2SMOD | I2S_MODE_MASTER_TX;
    stubSpi2.I2SPR = 48u;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef* handle, std::uint16_t* data,
                                                 const std::uint16_t length) {
    if (handle->State != HAL_I2S_STATE_READY) return HAL_ERROR;
    testDmaBuffer = data;
    testDmaLength = length;
    ++testTransmitCount;
    stubDma.CR = DMA_SxCR_EN;
    handle->State = HAL_I2S_STATE_BUSY_TX;
    stubSpi2.I2SCFGR |= 1u << 10u;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_I2S_DMAStop(I2S_HandleTypeDef* handle) {
    ++testStopCount;
    stubDma.CR = 0;
    handle->State = HAL_I2S_STATE_READY;
    stubSpi2.I2SCFGR &= ~(1u << 10u);
    return HAL_OK;
}
inline void HAL_DMA_IRQHandler(DMA_HandleTypeDef*) {}

// Timer 4 / WS2812 HAL surface used by the STM32 light backend tests.
struct RCC_TypeDef { std::uint32_t CFGR = 0; std::uint32_t PLLCFGR = 16; };
inline RCC_TypeDef testRcc;
#define RCC (&testRcc)
#define RCC_PLLCFGR_PLLM 0x3fu
#define RCC_CFGR_PPRE1 (7u << 10u)
#define RCC_CFGR_PPRE1_DIV1 (0u << 10u)
inline std::uint32_t testPclk1Hz = 48000000u;
inline std::uint32_t HAL_RCC_GetPCLK1Freq() { return testPclk1Hz; }

struct TIM_Base_InitTypeDef {
    std::uint32_t Prescaler{}, CounterMode{}, Period{}, ClockDivision{}, AutoReloadPreload{};
};
struct TIM_HandleTypeDef {
    void* Instance{};
    TIM_Base_InitTypeDef Init{};
    DMA_HandleTypeDef* hdma[8]{};
};
struct TIM_OC_InitTypeDef {
    int OCMode{}, Pulse{}, OCPolarity{}, OCFastMode{};
};
inline int testTim4;
inline int testDma0;
#define TIM4 (&testTim4)
#define DMA1_Stream0 (&testDma0)
#define GPIO_PIN_6 (1 << 6)
#define GPIO_AF2_TIM4 2
#define TIM_COUNTERMODE_UP 0
#define TIM_CLOCKDIVISION_DIV1 0
#define TIM_AUTORELOAD_PRELOAD_DISABLE 0
#define TIM_OCMODE_PWM1 1
#define TIM_OCPOLARITY_HIGH 1
#define TIM_OCFAST_DISABLE 0
#define TIM_CHANNEL_1 1
#define TIM_DMA_ID_CC1 1
#define DMA_CHANNEL_2 2
#define DMA_NORMAL 0
#define DMA_PRIORITY_HIGH 2
#define DMA1_Stream0_IRQn 0
#define __HAL_RCC_TIM4_CLK_ENABLE() ((void)0)
constexpr int HAL_DMA_STATE_READY = 1;
inline int HAL_DMA_GetState(DMA_HandleTypeDef*) { return HAL_DMA_STATE_READY; }
inline int testTimPwmInitCount = 0;
inline int testTimPwmStartCount = 0;
inline int testTimPwmStopCount = 0;
inline std::uint32_t* testTimPwmData = nullptr;
inline std::uint16_t testTimPwmLength = 0;
inline HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef*) {
    ++testTimPwmInitCount;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef*, TIM_OC_InitTypeDef*, int) {
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_TIM_PWM_Start_DMA(TIM_HandleTypeDef*, int, std::uint32_t* data, std::uint16_t length) {
    ++testTimPwmStartCount;
    testTimPwmData = data;
    testTimPwmLength = length;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_TIM_PWM_Stop_DMA(TIM_HandleTypeDef*, int) {
    ++testTimPwmStopCount;
    return HAL_OK;
}

// PA15 -> TIM2_CH1 hardware edge counter used by the GPIO I2S diagnostic.
struct TimerCounterRegisters {
    std::uint32_t CR1 = 0;
    std::uint32_t SMCR = 0;
    std::uint32_t CCER = 0;
    std::uint32_t CCMR1 = 0;
    std::uint32_t PSC = 0;
    std::uint32_t ARR = 0;
    std::uint32_t CNT = 0;
    std::uint32_t EGR = 0;
};
inline TimerCounterRegisters testTim2;
inline int testGpioA;
#define TIM2 (&testTim2)
#define GPIOA (&testGpioA)
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_TIM2_CLK_DISABLE() ((void)0)
#define GPIO_PULLDOWN 2
#define GPIO_AF1_TIM2 1
#define TIM_CCMR1_CC1S_0 (1u << 0u)
#define TIM_CCER_CC1E (1u << 0u)
#define TIM_SMCR_TS_2 (1u << 6u)
#define TIM_SMCR_TS_0 (1u << 4u)
#define TIM_SMCR_SMS_2 (1u << 2u)
#define TIM_SMCR_SMS_1 (1u << 1u)
#define TIM_SMCR_SMS_0 (1u << 0u)
#define TIM_EGR_UG (1u << 0u)
#define TIM_CR1_CEN (1u << 0u)
inline void HAL_GPIO_DeInit(void*, std::uint32_t) {}

#define GPIO_PIN_8 (1 << 8)
#define GPIO_MODE_OUTPUT_PP 2
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
inline void HAL_GPIO_WritePin(void*, std::uint32_t, int) {}
