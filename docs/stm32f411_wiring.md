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

## Optional 0.91-inch OLED

A 128x32 SSD1306 I2C display can show the active bank and weapon, FSM state. Connect **PB8 to SCL**, **PB9 to SDA**, plus **3.3 V and GND**. The default address is `0x3C`; it is configurable in the STM32 board config. Details are in [OLED wiring and runtime](oled_display.md).

## DevEBox onboard W25Q32

The DevEBox has onboard W25Q32 flash, but it is intentionally not part of the blaster storage backend yet. The supplied board image does not document that flash chip's MCU pin mapping, and the existing audio asset set is larger than the device anyway. External microSD is therefore the canonical STM32 asset store.

## I2S GPIO loopback diagnostic

Use the `stm32f411_blackpill_tests` or `stm32f411_devebox_tests` firmware.
Select **a** to start a 440 Hz generated tone and the TIM2 hardware edge
counter on **PA15**. Move **one** short jumper at a time:

| Test command | Jumper | Expected reading |
| --- | --- | --- |
| `w` | PB12 (WS/LRCLK) -> PA15 | ~44,100 Hz |
| `b` | PB13 (BCLK) -> PA15 | ~1,411,200 Hz |
| `d` | PB15 (I2S DATA) -> PA15 | Nonzero rising edges while tone plays |
| `q` | Remove jumper; stop test | Returns to diagnostics menu |

PA15 is used only as the temporary TIM2_CH1 capture input; trigger PA0,
ladder PA1, SD SPI1, and the LED TIM4 driver remain unchanged. Do **not**
connect output pins to each other, to a voltage supply, or to ground.
Keep the jumper short. No external power or resistor is necessary for a
3.3 V STM32-to-STM32 GPIO connection. The PA15 input is pulled down while
testing to suppress false edge counts when no jumper is connected.

The counter is implemented using TIM2 in external clock mode 1: it counts
rising edges without CPU interrupts or GPIO polling. A passing WS/BCLK check
proves those waveforms reach PA15 and have approximately the right frequency.
A DATA activity report only proves toggling; it does **not** validate I2S
framing, data integrity, amplifier shutdown, power, or speaker output.
If all lines test correctly but the speaker is silent, check the amplifier
VIN/GND/SD state and speaker wiring independently.

### Separating GPIO loopback faults from I2S setup failures

In diagnostic `a`, connect **PB8 -> PA15**, then enter `t`. The firmware
outputs 250 low-frequency GPIO pulses and counts them in TIM2. Approximately
250 pulses proves the jumper and TIM2 input are functioning independently of I2S.
Remove that jumper before reconnecting PB12, PB13, or PB15 to PA15.

Enter `s` to inspect live PLLI2S, I2S enable, transmit DMA request, stream enable,
NDTR movement, and AF5 mode of PB12/PB13/PB15. These hardware states are more
meaningful than the 'tone requested' log. Measure `w`, `b`, or `d` only after
confirming the reference test and the live hardware status.
