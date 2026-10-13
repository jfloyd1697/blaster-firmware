# Merged weapon library and calibrated LEDs

The binary runtime bank index is `assets/weapons/weapons_manifest.bin`.
The supplied SD-card ZIP contains all six banks and 58 compiled weapons:

| Bank | Count |
| --- | ---: |
| Blaster | 28 |
| Characters | 11 |
| Default | 7 |
| Farts | 8 |
| SciFi | 3 |
| Tools | 1 |

The original `behavior.json` files are retained in the firmware source tree for
comparison, but **are not** read by the embedded runtime. The exported SD package
contains only the binary runtimes and shared assets.

## Key behaviors

- C3-PO: red heartbeat lighting on healing/reload, alongside the original sound.
- Yoda and Darth Maul: hurt/death/heal state machines with their source sounds,
  character-specific hit lights, and the heart-heal animation.
- Aiai Fallout: audio charge/normal/big shots, compiled charge/shot light effects.
- Aiai Health: random hurt clips, death, healing, and state-linked lighting.
- Metroid Prime: three regular burst shots use a 180 ms low-intensity amber/cyan
  flash that decays on each shot; charged release keeps the stronger effect.
- Farts: eight independent sound profiles in the Farts bank.
- Legacy weapons: original firing/charging/reloading behavior migrated into FSM
  states and compatible binary audio and light commands.

## Light output correction

`core/lights/LightOutputCurve.h` defines a quadratic (gamma ~2.0) output
correction and per-channel brightness ceilings: R=94, G=59, B=88. The
STM32F411 WS2812 output applies this in the driver to all frames without
modifying editable sequence data. The lower green peak compensates for green's
higher perceived brightness. Tune these three constants after testing the
physical ring; no SD re-export is necessary for a new brightness calibration.

## Deployment

1. Flash this firmware (the PLLI2SM audio clock fix is retained).
2. Remove the previous `/assets/weapons` directory from the SD card, if any.
3. Extract the matching complete SD-card ZIP to the card root.
4. Check audio and lights on hardware. The C++ tests run with simulated STM32
   registers/peripherals and do not verify physical output.

For reproducible source export, use the supplied Weapon FSM Editor:

    uv run weapon-fsm-export-sd --all-banks --sd-root E:\\

The editor's `src/weapon_fsm_workspace/demos/legacy/MISSING_SOURCE_AUDIO.md`
identifies eight unavailable original recordings. Seven charge-cannon sounds
are synthesized placeholders; super_metroid reload uses the generic reload.
These cannot be described as recovered originals.
