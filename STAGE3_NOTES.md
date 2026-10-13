# Stage 3: deferred playback requests (incremental)

STM32 audio `playSound(..., false)` records the requested path; actual WAV open/seek and DMA startup happen in `update()` outside the fixed control tick. A new request replaces an older pending request and `stop()` cancels it. The synchronous `blocking=true` path performs startup directly (blocking diagnostic playback retains its original wait semantics).

`LightEngine::requestSequenceFile()` records a light request without reading SD. The next `prefetchNextFrame()` starts the sequence by reading its header and initial frame. Stops and subsequent requests cancel pending sequences.

**Important:** This stage is not hard real-time. Weapon equip still parses behavior and prepares WAV files synchronously inside the FSM. `audio->update()` and `LightEngine::prefetchNextFrame()` still use synchronous SdFat reads, so bad SD latency can still delay control. Neither an SD-DMA driver nor a centralized arbiter is implemented in this stage. No board compilation or real-hardware latency validation has been completed.
