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
    struct { int PLLI2SN{}, PLLI2SR{}; } PLLI2S{};
};
struct GPIO_InitTypeDef { int Mode{}, Pull{}, Speed{}, Alternate{}, Pin{}; };
inline int stubSpi2;
inline int stubDma;
inline int stubGpio;
#define SPI2 (&stubSpi2)
#define DMA1_Stream4 (&stubDma)
#define GPIOB (&stubGpio)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SPI2_CLK_ENABLE() ((void)0)
#define __HAL_RCC_DMA1_CLK_ENABLE() ((void)0)
#define __HAL_LINKDMA(handle, field, dma) ((handle)->field = &(dma))
#define RCC_PERIPHCLK_I2S 1
#define GPIO_MODE_AF_PP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_VERY_HIGH 2
#define GPIO_AF5_SPI2 5
#define GPIO_PIN_12 (1<<12)
#define GPIO_PIN_13 (1<<13)
#define GPIO_PIN_15 (1<<15)
#define I2S_MODE_MASTER_TX 1
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
inline HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(const RCC_PeriphCLKInitTypeDef*) { return HAL_OK; }
inline void HAL_GPIO_Init(void*, GPIO_InitTypeDef*) {}
inline HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef*) { return HAL_OK; }
inline void HAL_NVIC_SetPriority(int, int, int) {}
inline void HAL_NVIC_EnableIRQ(int) {}
inline HAL_StatusTypeDef HAL_I2S_Init(I2S_HandleTypeDef* handle) {
    handle->State = HAL_I2S_STATE_READY;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef* handle, std::uint16_t* data,
                                                 const std::uint16_t length) {
    if (handle->State != HAL_I2S_STATE_READY) return HAL_ERROR;
    testDmaBuffer = data;
    testDmaLength = length;
    ++testTransmitCount;
    handle->State = HAL_I2S_STATE_BUSY_TX;
    return HAL_OK;
}
inline HAL_StatusTypeDef HAL_I2S_DMAStop(I2S_HandleTypeDef* handle) {
    ++testStopCount;
    handle->State = HAL_I2S_STATE_READY;
    return HAL_OK;
}
inline void HAL_DMA_IRQHandler(DMA_HandleTypeDef*) {}

// Timer 4 / WS2812 HAL surface used by the STM32 light backend tests.
struct RCC_TypeDef { std::uint32_t CFGR = 0; };
inline RCC_TypeDef testRcc;
#define RCC (&testRcc)
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
