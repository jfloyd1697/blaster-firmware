# STM32F411 SPI2/I2S2 register recovery

The GPIO loopback diagnostic proved PA15/TIM2 can count edges from PB8, but WS/BCLK/DATA had no edges during playback. The sampled SPI2 registers were:

- PLLI2S ready: YES
- I2S enabled (`I2SE`): YES
- I2S mode (`I2SMOD`): SPI, not I2S
- TX DMA enabled, with `NDTR` static at 4095
- `I2SPR = 0x2` (default/reset divider)

A successful `HAL_I2S_Init` must configure `I2SMOD`, `I2SCFG=MASTER_TX` and the I2S divider before `HAL_I2S_Transmit_DMA` starts. The observed registers indicate the peripheral was never correctly initialized, or its configuration was reset afterward. The reason for the lost configuration has not yet been isolated.

Firmware changes:

- Verify I2S mode, master-transmit mode, and prescaler after `HAL_I2S_Init`.
- Before each playback, if the HAL handle is idle but the peripheral registers are invalid, invoke `HAL_I2S_Init` to reconfigure them. Fail if the recheck fails.
- Report mode loss during active playback instead of silently continuing.
- Only stop DMA when there is active playback.
- Show raw `I2SCFGR` and master-TX mode in diagnostic `a` / `s`.

On hardware: flash `stm32f411_blackpill_tests`, connect PB12->PA15 or PB13->PA15, run diagnostic `a` then `w`/`b` and `s` while tone is active. If recovery messages recur or frequencies remain 0, collect the new logs: the underlying peripheral overwrite or clock error still needs investigation. Never connect two output pins together.

## Diagnosing failed startup after enabling strict I2S checks

The platform deliberately retains the audio service object when its `begin()`
returns false, so diagnostic option `a` can still exercise it. Previously,
`ensureI2sReady()` rejected any HAL handle state other than `READY`, which
prevented any retry following the original initialization failure. The loopback
then reported `I2S2 HAL handle not ready for playback`, without testing recovery.

The audio backend now retries **full peripheral/DMA initialization** when the
HAL handle is not READY or the I2S mode register is invalid, provided DMA1
Stream 4 is not enabled. If initialization fails again, the log includes the
HAL state/error and I2S mode/prescaler register values. A successfully recovered
backend also rebinds the DMA-completion callbacks to the active instance.

### STM32 board check

1. Flash `stm32f411_blackpill_tests` and open serial monitor.
2. Capture the complete startup output, especially the audio service messages.
3. Select `a`. This now retries initialization after an earlier startup failure.
4. If initialization succeeds, test `s`, PB12→PA15 with `w` and PB13→PA15 with `b`.
5. If it still fails, preserve `HAL_I2S_Init failed`, `I2SCFGR`, `I2SPR`,
   `HAL state`, and `HAL error` from the startup and retry logs.

The host tests exercise both transient failed initialization and refusal to
reconfigure hardware while DMA is enabled. They do not verify on-board I2S.
