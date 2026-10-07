import json
import pathlib
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Union

from .model import WeaponBehavior


@dataclass
class WeaponEntry:
    weapon_id: str
    bank: str
    behavior: WeaponBehavior

    def behavior_path(self):
        return pathlib.Path(self.bank) / self.weapon_id / 'behavior.json'



@dataclass
class WeaponBuildSession:
    output_root: Path
    manifest_name: str = "weapons_manifest.json"
    entries: List[WeaponEntry] = field(default_factory=list)

    def __init__(self, output_root: Union[str, Path], manifest_name: str = "weapons_manifest.json") -> None:
        self.output_root = Path(output_root)
        self.manifest_name = manifest_name
        self.entries = []

    def add_behavior(self, bank: str, weapon_id: str, behavior: WeaponBehavior) -> "WeaponBuildSession":
        self._validate_weapon_id(weapon_id)
        self._replace_existing(bank=bank, weapon_id=weapon_id, behavior=behavior)
        return self

    def write_all(self) -> None:
        self.output_root.mkdir(parents=True, exist_ok=True)

        for entry in self.entries:
            behavior_path = (self.output_root /  entry.behavior_path())
            behavior_path.parent.mkdir(parents=True, exist_ok=True)
            behavior_path.write_text(
                json.dumps(entry.behavior.to_dict(), indent=2),
                encoding="utf-8",
            )

        manifest = self._build_manifest()
        manifest_path = self.output_root / self.manifest_name
        manifest_path.write_text(
            json.dumps(manifest, indent=2),
            encoding="utf-8",
        )

    def _build_manifest(self) -> Dict[str, object]:
        banks: "OrderedDict[str, List[str]]" = OrderedDict()

        for entry in self.entries:
            if entry.bank not in banks:
                banks[entry.bank] = []
            banks[entry.bank].append(entry.behavior_path().as_posix())

        return {
            "version": 1,
            "banks": [
                {
                    "name": bank_name,
                    "weapons": weapon_paths,
                }
                for bank_name, weapon_paths in banks.items()
            ],
        }

    def _replace_existing(self, bank: str, weapon_id: str, behavior: WeaponBehavior) -> None:
        for index, entry in enumerate(self.entries):
            if entry.weapon_id == weapon_id:
                self.entries[index] = WeaponEntry(
                    weapon_id=weapon_id,
                    bank=bank,
                    behavior=behavior,
                )
                return

        self.entries.append(
            WeaponEntry(
                weapon_id=weapon_id,
                bank=bank,
                behavior=behavior,
            )
        )

    @staticmethod
    def _validate_weapon_id(weapon_id: str) -> None:
        if not weapon_id:
            raise ValueError("weapon_id must not be empty")
        if "/" in weapon_id or "\\" in weapon_id:
            raise ValueError("weapon_id must be a single folder name, not a path")
