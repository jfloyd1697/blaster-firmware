import os
import pathlib

from tools.generate_weapon_json import ASSETS_ROOT
from weapon_builder.director import WeaponDirector
from weapon_builder.session import WeaponBuildSession


def main() -> None:
    director = WeaponDirector()
    session = WeaponBuildSession("assets/weapons")
    ASSETS_ROOT = pathlib.Path("../../../../assets").resolve()
    for sound in (ASSETS_ROOT / "audio"/ "blaster").iterdir():
        if sound.suffix == ".wav":
            session.add_behavior(
                bank="Blaster",
                weapon_id=sound.stem,
                behavior=director.build_single_shot(
                    weapon=sound.stem,
                    magazine_size=0,
                    fire_sounds=[str(sound.relative_to(ASSETS_ROOT))],
                ),
            )

    for sound in (ASSETS_ROOT / "audio"/ "fart").iterdir():
        if sound.suffix == ".wav":
            session.add_behavior(
                bank="Fart",
                weapon_id=sound.stem,
                behavior=director.build_single_shot(
                    weapon=sound.stem,
                    magazine_size=0,
                    fire_sounds=[str(sound.relative_to(ASSETS_ROOT))],
                ),
            )

    # session.add_behavior(
    #     bank="Default",
    #     weapon_id="plasma_pistol",
    #     behavior=director.build_single_shot(
    #         weapon="plasma_pistol",
    #         magazine_size=12,
    #         fire_sounds=[
    #             "assets/audio/weapons/plasma_pistol/fire_0.wav",
    #             "assets/audio/weapons/plasma_pistol/fire_1.wav",
    #         ],
    #         reload_sound="assets/audio/weapons/plasma_pistol/reload.wav",
    #     ),
    # )
    #
    # session.add_behavior(
    #     bank="Default",
    #     weapon_id="burst_rifle",
    #     behavior=director.build_burst(
    #         weapon="burst_rifle",
    #         magazine_size=30,
    #         fire_sounds=[
    #             "assets/audio/weapons/burst_rifle/shot_0.wav",
    #             "assets/audio/weapons/burst_rifle/shot_1.wav",
    #         ],
    #         reload_sound="assets/audio/weapons/burst_rifle/reload.wav",
    #         burst_count=3,
    #         step_delay_ms=80,
    #     ),
    # )
    #
    # session.add_behavior(
    #     bank="Default",
    #     weapon_id="auto_rifle",
    #     behavior=director.build_full_auto(
    #         weapon="auto_rifle",
    #         magazine_size=30,
    #         fire_sounds=[
    #             "assets/audio/weapons/auto_rifle/shot_0.wav",
    #             "assets/audio/weapons/auto_rifle/shot_1.wav",
    #         ],
    #         reload_sound="assets/audio/weapons/auto_rifle/reload.wav",
    #         fire_interval_ms=60,
    #     ),
    # )

    session.add_behavior(
        bank="Default",
        weapon_id="sawn_off",
        behavior=director.build_single_shot(
            weapon="sawn_off",
            magazine_size=3,
            fire_sounds=[
                "assets/weapons/sawnoff/30_sound_sawnOffFire.wav",
            ],
            reload_sound=[
                "assets/weapons/sawnoff/32_sound_sawnOffShellReload.wav",
                "assets/weapons/sawnoff/32_sound_sawnOffShellReload.wav",
                "assets/weapons/sawnoff/32_sound_sawnOffShellReload.wav",
                "assets/weapons/sawnoff/31_sound_sawnOffShellPump.wav",
            ]
        ),
    )

    session.add_behavior(
        bank="Default",
        weapon_id="shotgun",
        behavior=director.build_single_shot(
            weapon="shotgun",
            magazine_size=2,
            fire_sounds=[
                "assets/weapons/shotgun/12_sound_shotgunFire.wav",
            ],
            reload_sound=[
                "assets/weapons/shotgun/13_sound_shotgunPump.wav",
            ]
        ),
    )

    session.add_behavior(
        bank="SciFi",
        weapon_id="charge_cannon",
        behavior=director.build_charge_shot(
            weapon="charge_cannon",
            magazine_size=6,
            charge_start_sound="assets/audio/weapons/charge_cannon/charge_start.wav",
            charge_loop_sound="assets/audio/weapons/charge_cannon/charge_loop.wav",
            charge_fire_sounds=[
                "assets/audio/weapons/charge_cannon/fire_0.wav",
                "assets/audio/weapons/charge_cannon/fire_1.wav",
            ],
            charge_cancel_sound="assets/audio/weapons/charge_cannon/cancel.wav",
            charge_complete_sound="assets/audio/weapons/charge_cannon/charged.wav",
            reload_sound="assets/audio/weapons/charge_cannon/reload.wav",
            charge_time_ms=800,
        ),
    )

    session.add_behavior(
        bank="Tools",
        weapon_id="chainsaw",
        behavior=director.build_chainsaw(
            weapon="chainsaw",
            idle_sound="assets/weapons/chainsaw/26_sound_chainsawIdle.wav",
            working_sound="assets/weapons/chainsaw/26_sound_chainsawWorking.wav",
            end_sound="assets/weapons/chainsaw/24_sound_chainsawEnd.wav",
            end_duration_ms=800,
        ),
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="anakin_hurt",
        behavior=director.build_random_semi_auto(
            weapon="anakin_hurt",
            fire_sounds=[
                "assets/weapons/anakin_hurt/anakin_hurt0.wav",
                "assets/weapons/anakin_hurt/anakin_hurt1.wav",
                "assets/weapons/anakin_hurt/anakin_hurt2.wav",
            ],
            reload_sound="assets/audio/lego_util/anakin_hurt/heart_refill.wav",
            empty_sound="assets/weapons/anakin_hurt/anakin_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="padme_hurt",
        behavior=director.build_random_semi_auto(
            weapon="padme_hurt",
            fire_sounds=[
                "assets/weapons/padme_hurt/padme_hurt0.wav",
                "assets/weapons/padme_hurt/padme_hurt1.wav",
                "assets/weapons/padme_hurt/padme_hurt2.wav",
            ],
            reload_sound="assets/audio/weapons/anakin_hurt/heart_refill.wav",
            empty_sound="assets/audio/weapons/anakin_hurt/anakin_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.write_all()


if __name__ == "__main__":
    main()
