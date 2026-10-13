# Real-time scheduling changes (STM32F411)

The STM32 still uses one cooperative foreground loop; I2S and TIM4 PWM transmission use separate DMA channels. This patch reduces synchronous lighting I/O in that loop but is not a complete hard-real-time scheduler.

- I2S audio `update()` runs before light processing and reads from the SD card whenever a DMA half requires a refill. Audio half buffer is 1024 stereo samples (~23 ms at 44.1 kHz).
- `LightEngine::update()` no longer reads a streamed light frame from storage at its deadline. `prefetchNextFrame()` reads **at most one** frame, after the weapon FSM update, and caches it. A frame not ready at its deadline is held and a missed-prefetch counter increments. The first frame is retained for loop rewinds.
- Serial bridge receive handling consumes no more than 48 bytes per service pass, preventing a chatty host from starving control indefinitely.
- Audio ISR records attempts to reuse a DMA half that was still marked for refill. See `Stm32F411AudioBackend::refillDeadlineMisses()`.

## Important limitations

- This is not asynchronous SD I/O. Even one SdFat read may block on an SD card's worst-case latency. A hard upper bound on input latency is not yet guaranteed.
- Audio and lighting still share SD storage, and file opens on transitions may block.
- There is no RTOS task separation, DMA-led SPI SD service, timestamped instrumentation, or guaranteed zero underruns. Test on real STM32 hardware under simultaneous audio/light/serial load before claiming real-time deadlines.
- Compiled light format is unchanged. Hardware output brightness and PWM encoding remain in the MCU.

## Validation

Build portable tests on Windows with CMake/CTest. `LightStreamPerformanceTest` verifies the deadline path has no storage read, a missing prefetch holds the current frame, the prefetched frame advances correctly, and loop rewind is cached. The Linux host build cannot complete because the PC audio implementation uses `windows.h`.

## Stage 2: measured control schedule

- STM32 foreground control ticks are scheduled at a nominal 1 ms period using wrap-safe microsecond timestamps. Missed periods are counted and are **not** run in catch-up bursts.
- Input acquisition and `Blaster::update()` are invoked only on control ticks, before audio and lighting foreground work. Audio refill remains ahead of lighting prefetch and serial polling.
- Prefetch and serial polling only start when at least 400 us remains before the next control tick. This admission check is **not** a bound on their execution time.
- `@BRIDGE STATS` reports `control_ticks`, `missed_ticks`, `max_lateness_us`, `max_control_us`, `max_background_us`, `background_passes`, and `light_misses`.
- A platform-independent scheduler unit test is at `tests/ControlScheduleTest.cpp`.

### Still required before hard-real-time signoff

`Blaster::update()` may synchronously load assets during weapon selection or trigger effects. `services.audio->update()` still performs blocking SdFat reads. `bridgePoll()` may execute commands with synchronous asset opens. Those calls must be converted to deferred, bounded state machines or true asynchronous I/O before the stated input latency target can be guaranteed. A central SD arbiter should own file operations across consumers; this stage does not misrepresent priority ordering as asynchronous I/O. Validation on hardware must include adversarial SD latency and concurrent serial/audio/LED activity. No STM32 hardware build was available in the authoring environment.
