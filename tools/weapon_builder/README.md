# weapon-behavior-builder

A small installable Python package for generating weapon `behavior.json` files and a root `weapons_manifest.json` file.

## Features

- Low-level builder API for states, transitions, actions, and sequences
- Director recipes for common weapon types:
  - single shot
  - burst
  - full auto
  - charge shot
  - chainsaw / looping tool
  - anakin hurt / death sequence
- Stateful build session that accumulates multiple weapons and writes:
  - `assets/weapons/<weapon_id>/behavior.json`
  - `assets/weapons/weapons_manifest.json`

## Install

From the package folder:

```bash
pip install .
```

Editable install:

```bash
pip install -e .
```

## Example

```python
from weapon_behavior_builder import WeaponBuildSession, WeaponDirector

session = WeaponBuildSession("assets/weapons")
director = WeaponDirector()

session.add_behavior(
    bank="Default",
    weapon_id="chainsaw",
    behavior=director.build_chainsaw(
        weapon="chainsaw",
        idle_sound="assets/weapons/chainsaw/26_sound_chainsawIdle.wav",
        working_sound="assets/weapons/chainsaw/26_sound_chainsawWorking.wav",
        end_sound="assets/weapons/chainsaw/24_sound_chainsawEnd.wav",
        end_duration_ms=800,
    ),
)

session.write_all()
```

You can also run the packaged example generator:

```bash
weapon-behavior-examples
```
