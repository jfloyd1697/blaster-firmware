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

The STM32 backend uses a MAX98357A on SPI2/I2S with DMA, an external microSD card on SPI1, and a WS2812 output on PB6 using TIM4 PWM + DMA. Both board configurations use the same blaster harness pinout. See `docs/stm32f411_wiring.md`.

The LED count is configured independently per environment with `BLASTER_STM32F411_LED_COUNT`.


## Platform diagnostics

The hardware diagnostics are implemented once in `src/tests/PlatformDiagnostics.cpp` and run against the selected platform factory. The interactive serial menu includes input, raw ladder, storage, SD dump, generated tones, exported-WAV playback, and lights tests.

**Audio diagnostics** (STM32F411 and Teensy Audio Shield):

- **5 — Generated tone:** plays 440, 660, then 880 Hz for 350 ms each, with gaps. Repeats the three-tone sequence until **q** is pressed, then stops audio and returns to the menu. Audio is synthesized in the audio backend: **no SD file, physical button, or button-ladder reading is needed**. STM32 uses its existing I2S2 DMA path; Teensy uses `AudioSynthWaveformSine` through stereo mixers to the SGTL5000 Audio Shield.
- **8 — Exported WAV:** reads the compiled bank manifest and selects the first referenced audio file that exists on SD. Weapon asset tables are read in capped 1–8 KiB chunks rather than loading the entire FSM. This exercises SD, WAV parsing, and hardware playback. Requires a fresh SD export with compatible 44.1 kHz / 16-bit PCM audio.
- **9 — WAV by path:** prompts for the complete SD-relative path (for example `assets/weapons/Default/metroid_prime/audio/shot.wav`), checks it exists, then plays it. Press **q** to stop playback.

Option 5 reports if a target backend does not support synthesized tones. The ESP8266 backend does not yet implement this feature, although its file playback path is still available under options 8/9.

Examples:

```text
pio run -e stm32f411_blackpill_tests -t upload
pio device monitor -p COM5 -b 115200

# Other test firmware environments:
pio run -e stm32f411_devebox_tests -t upload
pio run -e teensy40_tests -t upload
pio run -e esp_tests -t upload
```

If the normal firmware cannot load or parse `assets/weapons/weapons_manifest.bin`, it now enters a built-in fallback mode instead of stopping. Fallback mode pulses the lights amber and flashes white when the trigger is pressed.

## Asset layout validation

The normal firmware validates the SD asset layout before loading weapon banks. Exported weapons use the compact `weapon.bin` runtime format. Boot validation reads only the binary asset table for each weapon, verifies every referenced audio/light file exists, and avoids constructing the complete FSM for every weapon. The complete FSM is decoded only when that weapon is equipped. Host-side full validation still decodes and validates the entire runtime.

The same validation is available from the platform diagnostics menu as `7 - asset layout test`.

The repository asset tree can be checked on the host with CTest:

```text
cmake -S . -B build -DBLASTER_PLATFORM=PC -DBUILD_TESTING=ON
cmake --build build --target AssetLayoutTest
ctest --test-dir build -R asset_layout --output-on-failure
```

## Compiled SD assets

Weapon, manifest, light sequence, and light layout assets use versioned binary formats.
See [Binary asset pipeline](docs/BINARY_ASSET_PIPELINE.md). The PC authoring exporter is
part of the `weapon-fsm-workspace` project. Embedded firmware does not parse YAML or JSON.

## STM32F411 audio and WS2812 board bring-up

The STM32F411 peripherals operate independently of the SD card and weapon files.
Flash `stm32f411_blackpill_tests`, reconnect the serial monitor after DFU reset,
and inspect startup logs **before** running option 5 (generated repeating tones)
or option 6 (red/green/blue/white LEDs).

The STM32 light backend computes TIM4 PWM timing from the running APB1 clock,
including the APB prescaler. It prints actual timer clock, bit rate and ticks/bit.
The audio backend prints whether clock setup, DMA initialization or I2S initialization failed.
`TIM4/DMA initialized` and `I2S2 DMA output initialized` mean HAL initialization succeeded,
not that correct waveforms and voltage levels have been electrically measured.

Wiring: MAX98357A LRC=PB12, BCLK=PB13, DIN=PB15, VIN=module-supported supply,
GND=STM32 GND, speaker on amp outputs (not STM32 GND), and amplifier shutdown/enable
must be inactive. WS2812 DIN=PB6, correct first-pixel DIN direction,
LED power supply sized appropriately, common GND. When LEDs use 5 V power,
consider a 3.3-to-5 V logic-level translator: 3.3 V logic high is not guaranteed
for every WS2812-type LED at 5 V VDD. Never drive addressable LEDs from a
microcontroller IO pin as the power supply.

If the software logs both initialized but no sound/light output, measure the
I2S BCLK/LRC/DIN signals and WS2812 DIN waveform at the peripheral pins. This
separates peripheral output faults from wiring, power, and signal-integrity issues.
The generated-tone test does not require SD or buttons; option 6 does not
require a compiled light sequence file.

Host C++ tests exercise mocked peripheral drivers only. They are not substitutes
for the target PlatformIO build or oscilloscope/hardware tests.

## 61-LED CPX light-sequence showcase diagnostic

Option **0** cycles continuously through all six animation layer types supported
by the editor—**solid**, **radial pulse**, **wipe**, **blink**, **chase**, and
**sparkle**—then plays the existing **rainbow**. Each effect runs for 3 seconds
at 20 FPS; the entire seven-effect showcase repeats indefinitely until the
serial command `q` stops it, clears the ring, and returns to the menu.

The diagnostic streams versioned `WFLSEQ01` frame data from SD in bounded
reads through the normal light engine. No physical buttons or weapon selection
are required. It validates that the compiled `WFLLAY01` layout and every
sequence header specify 61 LEDs before starting.

Copy these files from the ZIP's `assets/lights/` folder to the same SD paths:

```text
assets/lights/rainbow_ring_61.layout.bin
assets/lights/rainbow_ring_61.bin
assets/lights/diagnostics/solid.bin
assets/lights/diagnostics/radial_pulse.bin
assets/lights/diagnostics/wipe.bin
assets/lights/diagnostics/blink.bin
assets/lights/diagnostics/chase.bin
assets/lights/diagnostics/sparkle.bin
```

The 61-LED layout geometry used to generate the six effect fixtures is a
PC-side source file at `tools/light_showcase/cpx_ring_61.json`. This JSON is
**not needed on the SD card**. The **radial pulse** now expands from the
center LED through all four rings and contracts back to the center over three
seconds. The radial fill uses the layout coordinates and stays illuminated across all
reached rings; the wipe progresses left-to-right, keeping passed LEDs lit
rather than displaying a traveling stripe. To regenerate just this binary (without changing other effects):

```powershell
python tools/generate_light_showcase.py --effect radial_pulse
```

Omit `--effect radial_pulse` to regenerate all six diagnostic sequences.
Run `python -m unittest discover -s tests -p test_light_showcase_generator.py`
to verify ring timing and the generated binary file.

All STM32F411 PlatformIO environments specify
`BLASTER_STM32F411_LED_COUNT=61`. This value must match the physical LED
count for the current hardware DMA backend.

On the Black Pill:

```powershell
pio run -e stm32f411_blackpill_tests -t upload
pio device monitor -p COM5 -b 115200
```

Select `0` to begin the full looping sequence, and `q` to stop. Option `6`
remains the separate red/green/blue/white test.

Host-side `PlatformDiagnosticsTest` verifies the sequence order, playback
through the real compiled fixtures, whole-showcase wrap, stop/clear, and
restart. Physical hardware output still needs board-level validation.

## 61-LED radial diagnostics

The diagnostic radial pulse is a center-out, outer-in **filled ring** effect, not a traveling band. For hardware index mapping verification, see [Ring mapping diagnostic](docs/RADIAL_RING_MAPPING.md).

## Corrected wipe diagnostic

The `wipe` stage now fills the ring from left to right, keeping every passed
LED lit until the sequence ends. This replaces the old narrow traveling
stripe. Copy `assets/lights/diagnostics/wipe.bin` to the same SD path; firmware
reflashing is not necessary for this asset-only change.

If the wipe still appears to snake or chase across the real strip, the
`cpx_ring_61` index-to-physical-coordinate mapping needs to be verified.
The ring-identification test in `docs/RADIAL_RING_MAPPING.md` can distinguish
that mapping error from the animation formula.
