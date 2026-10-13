# Stage 4 — foreground storage arbitration and deferred STM32 weapon equip

This stage modifies the Stage 3 project in place and preserves the existing audio/LED DMA implementations.

## STM32 changes

- Weapon selection and initial construction mark weapon loading pending instead of opening/parsing the behavior binary and preparing WAV files inside the 1 ms control tick. Repeated selections coalesce to the currently selected weapon. The old controller and active outputs are stopped immediately; inputs cannot fire an old behavior while the new weapon is pending.
- Outside the control tick, the foreground dispatcher services audio first, then one pending weapon load, then light prefetch, then serial polling, subject to the existing pre-operation slack check. The weapon load runs at most once per pending request, but may include several blocking SD transactions.
- `@BRIDGE STATS` adds `max_audio_us`, `max_weapon_load_us`, and `weapon_loads` to the existing scheduler timing metrics. `max_background_us` continues to cover lighting prefetch. Use these measurements under simultaneous playback, lighting, and serial workload.
- PC, ESP, and Teensy keep immediate weapon loading behavior.

## Timing and safety limitations

This **does not** implement a true nonblocking SD driver or preemptive arbitration. Audio refill, weapon loading, light prefetch, and some serial bridge commands still contain synchronous operations. The dispatcher checks slack only *before* starting these calls; a slow SD operation can overrun one or more 1 ms ticks. The control schedule remains best-effort and hard input latency cannot be guaranteed yet. STM32 code must be built and tested on the board before deployment.

## Validation run

- Host CMake target builds: `WeaponSwitchCleanupTest`, `WeaponRuntimeTest`, and `LightStreamPerformanceTest` passed.
- Host CTest cases: `weapon_switch_cleanup`, `weapon_runtime`, `light_stream_performance` passed.
- STM32 Blaster compilation unit passed a host C++17 syntax check with `PLATFORM_STM32F411` defined.
- Full Linux PC app build is unavailable due to pre-existing `windows.h` dependency in `PCAudioBackend.cpp`.
- STM32 PlatformIO board compilation and hardware timing validation were not available here.

## Further work for hard real-time

An actual asynchronous SD SPI transaction state machine (with DMA as supported by the board/driver, bounded bus ownership, readiness signaling and an explicit storage request queue) is still needed. Refactor the WAV refill, behavior loader and lighting parser onto that API. The serial bridge must not perform file operations synchronously. An interrupt-level input timestamp or a high-priority timer-driven control task would be needed to demonstrate a bound independent of worst-case card latency.
