import json
import os
from pathlib import Path


ASSETS_ROOT = Path("../assets").resolve()
OUTPUT_FILE = Path("../assets/audio/weapon_profiles.json")

def make_weapon(name, path):
    return {
        "name": name,
        "sounds": {
            "shot": str(path.resolve().relative_to(ASSETS_ROOT.parent)).replace("\\", "/"),
            # "reload": "",
            # "empty": "",
            # "chargeStart": "",
            # "chargeLoop": "",
            # "chargeRelease": ""
        },
        "shootMode": {
            "type": "single",
            # "cooldownMs": 120,
            # "burstCount": 3,
            # "burstIntervalMs": 60,
            # "minChargeMs": 300,
            # "maxChargeMs": 1200
        },
        # "ammoPerShot": 1,
        # "magazineSize": 0,
    }


def build_bank(folder: Path):
    weapons = []
    import mp3_to_wav

    for file in sorted(folder.iterdir()):
        if not file.is_file():
            continue

        if file.suffix.lower() not in [".wav", ".mp3"]:
            continue

        if file.suffix == ".mp3":
            mp3_to_wav.convert(file, file.with_suffix(".wav"))
            continue

        name = file.stem.replace("_", " ").title()

        weapons.append(make_weapon(name, file))

    return {
        "name": folder.name,
        "weapons": weapons
    }


def main():
    banks = []

    if not ASSETS_ROOT.exists():
        print(f"Assets folder not found: {ASSETS_ROOT}")
        return

    for folder in sorted((ASSETS_ROOT / "audio\\weapons").iterdir()):
        if folder.is_dir():
            bank = build_bank(folder)

            if bank["weapons"]:
                banks.append(bank)

    output = {
        "banks": banks
    }

    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)

    print(f"Generated {OUTPUT_FILE}")
    print(f"Banks: {len(banks)}")


if __name__ == "__main__":
    main()