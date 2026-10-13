# Equipped-weapon audio preparation

The selected weapon's `WeaponBehaviorDef.clips` table is passed to the audio engine
at equip time, including random/effect clip definitions. Duplicate paths are
removed. When a weapon or bank changes, the previous sound and lighting are
stopped and the old audio catalog is released, including after failed weapon
loads.

On STM32F411, `Stm32F411AudioBackend::prepareSounds()` opens all distinct WAV
files, parses and validates their headers once, then retains the file handles
and PCM offsets while that weapon is selected. A shot seeks an already-open
file to its data offset and fills the existing I2S/DMA buffers; it does **not**
reopen the file or reparse its RIFF header. Replays, random sounds, and looped
sounds reuse these prepared descriptors. `stop()` does not release them;
`releasePreparedSounds()` does.

The full PCM contents remain on SD and are streamed. Storing all weapon WAVs
in the STM32F411's 128 KiB RAM is not feasible. This change reduces playback
setup work but still incurs the normal SD reads for streaming and a one-time
cost when equipping a new weapon. Missing/invalid WAVs are reported during
equip; the remaining prepared sounds remain usable.

Other backends keep their existing playback paths; their default preparation
hook is a no-op. The core and STM32 implementation are intentionally separate.

Host tests cover the core equip/release lifecycle and the STM32 prepared-file
path (repeated shots, distinct clips, looped playback, normal DMA completion,
and invalid/missing files). PlatformIO device playback must still be measured
on the target board.
