# STM32F411CE wiring

The STM32 implementation supports both of the current F411CE boards through board configuration data:

- `STM32F4x1Cx v2.0+` / Black-Pill-style board
- DevEBox STM32F411CE board

Both use the same external peripherals and pin allocation so the blaster harness can move between boards without being rewired.

## MAX98357A audio

The STM32 backend drives the MAX98357A with SPI3 in I2S master-transmit mode and DMA.

| STM32F411 | MAX98357A |
| --- | --- |
| PA4 | LRC / WS |
| PC10 | BCLK |
| PC12 | DIN |
| 5 V | VIN |
| GND | GND |

The backend supports the WAV formats currently present in the project: 16-bit PCM, mono or stereo, at 22.05 kHz or 44.1 kHz. Mono files are duplicated to left and right channels.

## External microSD

A microSD module is required on the STM32 boards. Assets retain the same `assets/...` paths used by the other platforms.

| STM32F411 | microSD |
| --- | --- |
| PB12 | CS |
| PB13 | SCK |
| PB14 | MISO |
| PB15 | MOSI |
| 3.3 V | VCC |
| GND | GND |

The SD bus uses SPI2 pins and is separate from the SPI3 peripheral used as I2S audio.

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
