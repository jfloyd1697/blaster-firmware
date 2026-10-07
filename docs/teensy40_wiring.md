# Teensy 4.0 + Audio Shield wiring

## Audio Shield Rev D / D2

The Teensy Audio Shield is used directly for audio output and storage.

| Function | Teensy 4.0 pin |
|---|---:|
| I2S data out | 7 |
| I2S data in | 8 |
| I2S LRCLK | 20 |
| I2S BCLK | 21 |
| I2S MCLK | 23 |
| SGTL5000 SDA | 18 |
| SGTL5000 SCL | 19 |
| SD CS | 10 |
| SD MOSI | 11 |
| SD MISO | 12 |
| SD SCK | 13 |

Place the existing `assets/` directory at the root of the Audio Shield microSD card.

## Lights

The Teensy implementation drives addressable WS2811/WS2812-compatible LEDs with OctoWS2811 DMA so LED updates do not rely on long interrupt-disabled bit banging while audio is active.

- Data: pin 2 by default
- Ground: common with Teensy and the LED power supply
- LED power: use an appropriately sized external supply for the strip
- Configure the number of LEDs with `BLASTER_TEENSY_LED_COUNT` in `platformio.ini`

Do not power a substantial LED strip from the Teensy 3.3 V rail.

## Inputs

- Trigger: pin 0, active-low with the Teensy internal pull-up
- Next / previous / reload resistor ladder: A0

The Audio Shield optional volume potentiometer uses A1/pin 15, so the input ladder is intentionally left on A0.
