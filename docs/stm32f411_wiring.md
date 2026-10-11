# STM32F411CE wiring

The STM32 implementation supports both of the current F411CE boards through board configuration data:

- `STM32F4x1Cx v2.0+` / Black-Pill-style board
- DevEBox STM32F411CE board

Both use the same external peripherals and pin allocation so the blaster harness can move between boards without being rewired.

## MAX98357A audio

The STM32 backend drives the MAX98357A with SPI2 in I2S master-transmit mode and DMA.

| STM32F411 | MAX98357A |
| --- | --- |
| PB12 | LRC / WS |
| PB13 | BCLK |
| PB15 | DIN |
| 5 V | VIN |
| GND | GND |

The STM32 I2S backend consumes **44.1 kHz, 16-bit PCM WAV** (mono or stereo). Mono files are duplicated to both output channels. The Weapon FSM Editor SD exporter uses ffmpeg to normalize authored sounds to **44.1 kHz mono PCM**; do not copy arbitrary WAV source files directly to the SD card.

## External microSD

A microSD module is required on the STM32 boards. Assets retain the same `assets/...` paths used by the other platforms.

| STM32F411 | microSD |
| --- | --- |
| PA4 | CS |
| PA5 | SCK |
| PA6 | MISO |
| PA7 | MOSI |
| 3.3 V | VCC |
| GND | GND |

The SD bus uses SPI1 pins and is separate from the SPI2 peripheral used as I2S audio.

## WS2812 / NeoPixel lights

| STM32F411 | WS2812 |
| --- | --- |
| PB6 | DIN |
| 5 V | 5 V power |
| GND | GND |

PB6 is TIM4 channel 1. The driver uses TIM4 PWM plus DMA1 Stream 0 / Channel 2, so LED signaling does not bit-bang the CPU while audio is running.

Use a 3.3 V to 5 V logic-level buffer if the LED strip is not reliable with a 3.3 V data-high level.

## Inputs

| Function | STM32F411 |
| --- | --- |
| Trigger | PA0, active-low with internal pull-up |
| Analog button ladder | PA1 |

The STM32 ADC is 12-bit, so the button-ladder thresholds are scaled for the 0-4095 ADC range.

## DevEBox onboard W25Q32

The DevEBox has onboard W25Q32 flash, but it is intentionally not part of the blaster storage backend yet. The supplied board image does not document that flash chip's MCU pin mapping, and the existing audio asset set is larger than the device anyway. External microSD is therefore the canonical STM32 asset store.
