# Optional 0.91-inch I2C OLED status display

The blaster supports the common **SSD1306 128x32 monochrome, four-pin I2C** OLED modules (often sold as 0.91-inch OLEDs). The default 7-bit I2C address is `0x3C`. This driver is *not* an SH1106 or 128x64 driver.

## Wiring

| OLED | STM32F411 BlackPill / DevEBox | Teensy 4.0 / 4.1 + Audio Shield |
| --- | --- | --- |
| GND | GND | GND |
| VCC | 3.3 V | 3.3 V |
| SCL | **PB8** (I2C1) | **19** (shared with Audio Shield SGTL5000) |
| SDA | **PB9** (I2C1) | **18** (shared with Audio Shield SGTL5000) |

Use a display breakout that operates at 3.3 V on both power and logic. Check the *actual* module pin labels; four-pin OLED headers are not all ordered identically. I2C needs pull-ups to 3.3 V (commonly already fitted to the OLED or Audio Shield). Add appropriate pull-ups only when absent.

The STM32 pin/address assignment is a regular optional `Stm32F411OledPins` field in `Stm32F411BoardConfig`; update it if another I2C harness is used. The Teensy uses its shared `Wire` bus and `BLASTER_OLED_ADDRESS`, defined in `platformio.ini`. Disable probing with `-DBLASTER_OLED_ENABLED=0` in the board's build flags if desired.

The normal blaster firmware will still run with no OLED installed. The driver probes the configured address at startup and remains disabled when no device responds. An I2C error during runtime disables further display writes, rather than retrying inside the real-time loop.

## Display contents

Four 8-pixel-high rows show:

1. Current bank number/name (e.g. `B2/6 BLASTER`).
2. Current weapon's name, scrolling at a moderate rate when longer than 21 characters.
3. Weapon position and load state (`LOADING`, `READY`, `ERROR`); in PC-remote modes this row identifies `PC INPUT` or `PC DIRECT`.
4. Weapon FSM state; for True Jedi, stud progress with a small progress bar.

The display does not claim to show battery charge without a battery-monitor interface: the existing BMS indicator LEDs remain separate.

## Real-time scheduling

The weapon/controller publishes an in-memory status snapshot. Framebuffer generation and I2C writes happen only in the foreground display service. The display uses a 512-byte framebuffer and sends **at most eight pixel bytes per background call** (or one short addressing command). On STM32 the display is serviced after the audio, lighting-prefetch, and serial-bridge work, and only when the 1 ms control schedule has adequate slack. OLED updates are therefore best-effort and may temporarily lag during heavy audio or asset loading; they never block the control tick deliberately.

The code uses the Arduino `Wire` library and a built-in uppercase 5x7 font; no graphics library is required.

## Validation

On a desktop host with CMake and a C++17 compiler, run the tests:

`cmake -S . -B build -DBLASTER_PLATFORM=PC -DBUILD_TESTING=ON`

`cmake --build build --target OledDisplayTest WeaponSwitchCleanupTest`

`ctest --test-dir build -R 'oled_display|weapon_switch_cleanup' --output-on-failure`

To flash STM32 or Teensy, use the matching PlatformIO environment. Verify that the startup log reports `SSD1306 OLED ready`. An actual-device I2C check is still required; the desktop tests validate rendering, bounded writes, failures and weapon-state publishing, not electrical behavior.
