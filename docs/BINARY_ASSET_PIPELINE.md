# Compiled SD asset pipeline

The host Weapon FSM Editor is responsible for authoring YAML/JSON and exporting
runtime binary files. Firmware never loads these authoring formats.

| Asset | Runtime path | Magic | Access |
|---|---|---|---|
| Weapon index | `assets/weapons/weapons_manifest.bin` | `WFMAN001` | once at boot |
| Weapon FSM | `assets/weapons/<bank>/<weapon>/weapon.bin` | `WFSMBIN1` | active weapon on equip |
| Light sequence | `.../lights/<sequence>.bin` | `WFLSEQ01` | frame-by-frame SD reads |
| Light layout | `.../lights/<sequence>.layout.bin` | `WFLLAY01` | validated metadata |
| Audio | `.../audio/*.wav` | 44.1 kHz, 16-bit PCM WAV | streamed by audio backend |

Embedded startup performs reference-only checks: binary magic/version, asset
references and sequence/layout headers. Full test validation also parses every
light frame and validates the complete weapon FSM. The light engine retains only
the current animation frame while playing a compiled sequence, never the full
animation. An animation can be larger than total MCU RAM.

The per-sequence light layout contains index and normalized geometry from the
authoring layout. Animations are already compiled to physical LED indices, so
firmware does **not** need the coordinates to render frames. The compiled layout
is checked for presence/version and matching LED count.

The editor SD exporter requires **ffmpeg** on PATH and converts all authored audio to 44.1 kHz, mono, signed 16-bit PCM WAV. This conversion happens on the PC and does not change source assets. Export errors stop that weapon from being added to the manifest.

The firmware rejects legacy `weapon.json` and `weapons_manifest.json` formats.
To deploy a new export, replace the SD `/assets/weapons` tree with freshly
compiled output, then reset the board. Older JSON assets are not compatible.

Tests:

```bash
cmake -S . -B build-tests -DBLASTER_PLATFORM=PC -DBUILD_TESTING=ON
cmake --build build-tests --target AssetLayoutTest WeaponRuntimeTest CompiledAssetsTest LightEngineTest
ctest --test-dir build-tests --output-on-failure
```

The compiled-asset C++ test consumes Python-generated fixtures in
`tests/data/compiled_assets`. They can be regenerated from the editor workspace
with `PYTHONPATH=src python tests/generate_firmware_fixtures.py <firmware>/tests/data/compiled_assets`.
