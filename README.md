# Updated C++ weapon behavior bundle

This bundle captures the current direction discussed in chat:

- `weapons_manifest.json` points directly to per-weapon `behavior.json` files
- `weapon_behavior` library stays focused on behavior parsing + validation
- `core` owns the runtime/controller, load helpers, and manifest/weapon loading
- looping audio is supported through `playSound(file, loop)` down into the backend
- `schedule_event` is supported in the controller for simple timed transitions

## Assumptions you may need to align

A few interfaces were inferred from the conversation and may need small edits to match your repo exactly:

- `ITime` exposes `millis() const`
- `ILights` exposes:
  - `setPattern(std::shared_ptr<weapon_behavior::LightPatternDef>)`
  - `flash()`
- `IDebug` exposes `log(std::string)` and `error(std::string)`
- `IInput` exposes:
  - `wasTriggerPressed()`
  - `wasTriggerReleased()`
  - `isTriggerHeld()`
  - `wasReloadPressed()`
  - `wasNextShortPressed()`
  - `wasPrevShortPressed()`
  - `wasQuitPressed()`
- `PlatformServices` has `std::shared_ptr` fields for `audio`, `debug`, `input`, `lights`, `time`, and `textLoader`

## Included example

- `assets/weapons/weapons_manifest.json`
- `assets/weapons/chainsaw/behavior.json`


## Teensy 4.0 + Audio Shield

A second embedded platform is available as the PlatformIO `teensy40` environment.

- Audio uses the PJRC Audio Library and the SGTL5000 on the Rev D/D2 Audio Shield.
- Assets are read from the Audio Shield microSD card (`CS=10`, SPI pins 11/12/13).
- WS2812/WS2811 lights use OctoWS2811 DMA on pin 2 by default.
- The LED strip length is configured with `BLASTER_TEENSY_LED_COUNT` in `platformio.ini`.
- Trigger defaults to pin 0 and the existing analog button ladder defaults to A0.

The SD card should retain the repository asset layout, with `assets/` at the card root.


## STM32F411CE

Two STM32F411CE board configurations are available:

- `stm32f411_blackpill` for the STM32F4x1Cx v2.0+ / Black-Pill-style board.
- `stm32f411_devebox` for the DevEBox STM32F411CE board.

The STM32 backend uses a MAX98357A on SPI3/I2S with DMA, an external microSD card on SPI2, and a WS2812 output on PB6 using TIM4 PWM + DMA. Both board configurations use the same blaster harness pinout. See `docs/stm32f411_wiring.md`.

The LED count is configured independently per environment with `BLASTER_STM32F411_LED_COUNT`.
