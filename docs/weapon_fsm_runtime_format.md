# Weapon FSM runtime deployment format

The embedded firmware does not parse authoring YAML or JSON. The Weapon FSM
Editor SD exporter compiles weapon FSMs, light animations, and light layouts to
versioned binary assets. Audio is converted on the host to 44.1 kHz, mono,
16-bit PCM WAV and streamed from SD at runtime.

Expected SD layout:

```text
assets/
  weapons/
    weapons_manifest.bin
    <bank>/
      <weapon>/
        weapon.bin
        audio/*.wav
        lights/<sequence>.bin
        lights/<sequence>.layout.bin
```

The binary manifest indexes weapon paths. At boot the firmware validates asset
references and file headers without constructing all weapon FSMs or loading
entire light animations. An equipped weapon is decoded when needed; light
sequence frames are streamed. Legacy `weapon.json` files are not supported.

See `BINARY_ASSET_PIPELINE.md` for format identifiers and test coverage.
