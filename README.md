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

The normal firmware now boots by loading only the compact `weapons_manifest.bin` bank index. It does **not** scan every weapon, sound, or lighting file at startup. The selected `weapon.bin` is decoded and validated when equipped. If the manifest cannot be loaded, the existing fallback mode is still used.

Run manual validation from the platform diagnostics menu using `7 - asset layout test`; the host-side `AssetLayoutTest` also remains available. The recursive SD storage tree dump is likewise available only on demand as diagnostic option 4, not automatically at startup.

### Runtime performance improvements

- STM32 button ladder performs one ADC conversion per input-update cycle (shared by all three ladder buttons); `AnalogButtonLadderConfig` is still the only threshold configuration.
- Light-sequence playback rasterizes a frame only when advancing to a new frame or after an override effect, instead of rebuilding an identical LED buffer on every main-loop iteration.
- Typical 61-LED compiled frames are fetched in one bounded SD range read rather than two separate file opens. STM32 SD reads use block transfers instead of byte-by-byte reads.
- Repeated held-trigger self-transitions no longer flood USB serial logs. FSM event processing and shot timing are unchanged.

These changes reduce unnecessary runtime work; device-level latency and audio-under-load still require measurement on the STM32 board.

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

## GPIO I2S electrical loopback (STM32 diagnostics option a)

Option **a** generates a repeating 440 Hz I2S test tone and uses the STM32F411
TIM2 external edge counter to measure PA15. Use a single jumper, testing one
I2S output at a time: PB12->PA15 and `w` (~44.1 kHz), PB13->PA15 and `b`
(~1.4112 MHz), PB15->PA15 and `d` (DATA toggling). Change the selected signal
with `w`/`b`/`d`; press `q` to stop and release PA15. This requires no WAV
file, SD card contents, button, scope, or MAX98357A. See
`docs/stm32f411_wiring.md` for wiring details and limitations.

## STM32 audio initialization retry (diagnostic update)

An STM32 audio service may be retained even when its first `begin()` fails.
The `a` diagnostic now attempts a safe full I2S2/DMA reinitialization before
starting the tone, instead of rejecting a non-READY HAL handle without a retry.
Initialization failures include the HAL handle state/error and the peripheral
register state in the serial log. If this diagnostic still cannot start, capture
both the audio-related **boot messages** and the messages printed by `a`.
See `docs/stm32f411_i2s_register_recovery.md` for the procedure.

### Standalone STM32F411 I2S hardware check

The STM32F411 I2S configuration was missing `PLLI2SM` (the I2S PLL input
prescaler). This causes `HAL_I2S_Init()` to report error 16 (prescaler
calculation error) before SPI2 is put into I2S mode. The normal audio backend
now uses the STM32duino system PLL input divider and explicitly chooses PLLI2S
as the peripheral clock source.

For a smaller, independent hardware test, flash one of:

```powershell
pio run -e stm32f411_blackpill_i2s_example -t upload
# or
pio run -e stm32f411_devebox_i2s_example -t upload
```

After USB CDC reconnects, open the serial monitor at 115200 baud. The example
runs a continuous 440 Hz tone with *blocking* HAL I2S transmission. It does
not initialize DMA, SD storage, button inputs, light drivers, or the rest of
the blaster firmware. Its code is `src/tests/Stm32I2sMinimalExample.cpp`.

The I2S output pins are PB12 = WS/LRC, PB13 = BCLK, PB15 = DATA. On a
44.1-kHz stereo 16-bit bus, expected WS is approximately 44.1 kHz and BCLK
approximately 1.4112 MHz. A compatible MAX98357A can be connected to the
three pins to test physical sound output. The standalone sketch intentionally
uses an attenuated sine wave to avoid starting at maximum audio level.

Restore the application/diagnostics by flashing
`stm32f411_blackpill_tests` (or the corresponding normal environment).

The host CMake/CTest suite includes `Stm32I2sClockConfigTest` to guard against
omitting the divider again. Tests run on host mocks; on-board I2S signals and
sound output still require hardware verification.

