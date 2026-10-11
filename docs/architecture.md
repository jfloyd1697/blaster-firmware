# Firmware architecture

The project is split into **behavior/core policy** and **platform hardware adapters**.

## Core

`lib/core` owns behavior that must be identical on every target:

- weapon FSM loading/execution
- audio engine lifecycle and the `IAudioBackend` contract
- light pattern timing, flash/pulse/sequence playback and the `ILightBackend` contract
- button debouncing, short/long press semantics and analog button-ladder decoding
- asset validation and storage-tree dumping
- platform-neutral service interfaces

The core must not include Arduino, STM32 HAL, Teensy Audio, ESP8266, Win32, GPIO numbers, DMA streams or filesystem-driver headers.

## Platform folders

A platform folder contains only code required to touch that platform:

- GPIO/ADC reads
- clocks/timers/DMA/I2S
- audio codec/output implementation
- LED transport implementation
- SD/filesystem adapter
- serial/debug transport
- wall-clock implementation
- board pin configuration

Platform light backends receive already-rendered RGB pixels. They do not implement flash, pulse or animation timing.

Platform audio backends receive play/stop requests. Audio lifecycle and the public audio interface live in core.

The analog button ladder thresholds and button mapping live in core; platform input classes only acquire the ADC value and platform-specific digital inputs.
