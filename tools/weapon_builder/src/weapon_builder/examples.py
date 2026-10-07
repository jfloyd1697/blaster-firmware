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
                    fire_sounds=[str(sound.relative_to(ASSETS_ROOT.parent))],
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
                    fire_sounds=[str(sound.relative_to(ASSETS_ROOT.parent))],
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
    session.add_behavior(
        bank="Default",
        weapon_id="slave_one",
        behavior=director.build_full_auto(
            weapon="slave_one",
            magazine_size=0,
            fire_sounds=[
                "assets/audio/slave1/slave1_shoot0.wav",
                "assets/audio/slave1/slave1_shoot1.wav",
                "assets/audio/slave1/slave1_shoot2.wav",
            ],
            fire_interval_ms=60,
        ),
    )

    session.add_behavior(
        bank="Default",
        weapon_id="sawn_off",
        behavior=director.build_single_shot(
            weapon="sawn_off",
            magazine_size=3,
            fire_sounds=[
                "assets/audio/sawn_off/30_sound_sawnOffFire.wav",
            ],
            reload_sound=[
                "assets/audio/sawn_off/32_sound_sawnOffShellReload.wav",
                "assets/audio/sawn_off/32_sound_sawnOffShellReload.wav",
                "assets/audio/sawn_off/32_sound_sawnOffShellReload.wav",
                "assets/audio/sawn_off/31_sound_sawnOffPump.wav",
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
                "assets/audio/shotgun/12_sound_shotgunFire.wav",
            ],
            reload_sound=[
                "assets/audio/shotgun/13_sound_shotgunPump.wav",
            ]
        ),
    )

    session.add_behavior(
        bank="SciFi",
        weapon_id="charge_cannon",
        behavior=director.build_charge_shot(
            weapon="charge_cannon",
            magazine_size=6,
            charge_start_sound="assets/audio/charge_cannon/charge_start.wav",
            charge_loop_sound="assets/audio/charge_cannon/charge_loop.wav",
            charge_fire_sounds=[
                "assets/audio/charge_cannon/fire_0.wav",
                "assets/audio/charge_cannon/fire_1.wav",
            ],
            charge_cancel_sound="assets/audio/charge_cannon/cancel.wav",
            charge_complete_sound="assets/audio/charge_cannon/charged.wav",
            reload_sound="assets/audio/charge_cannon/reload.wav",
            charge_time_ms=800,
        ),
    )

    session.add_behavior(
        bank="SciFi",
        weapon_id="super_metroid",
        behavior=director.build_charge_shot(
            weapon="super_metroid",
            magazine_size=0,
            charge_start_sound="assets/audio/super_metroid_gun/beam_charge_start.wav",
            charge_loop_sound="assets/audio/super_metroid_gun/beam_charge_loop.wav",
            charge_fire_sounds=[
                "assets/audio/super_metroid_gun/beam_shot.wav",
            ],
            reload_sound="assets/audio/super_metroid_gun/reload.wav",
            charge_time_ms=800,
        ),
    )

    session.add_behavior(
        bank="SciFi",
        weapon_id="shinespark",
        behavior=director.build_charge_shot(
            weapon="shinespark",
            magazine_size=0,
            charge_start_sound="assets/audio/shinespark/shinespark_start.wav",
            charge_loop_sound="assets/audio/shinespark/shinespark_loop.wav",
            charge_fire_sounds=[
                "assets/audio/shinespark/shinespark_stop.wav",
            ],
            charge_time_ms=800,
        ),
    )

    session.add_behavior(
        bank="SciFi",
        weapon_id="shinespark",
        behavior=director.build_charge_shot(
            weapon="shinespark",
            magazine_size=0,
            charge_start_sound="assets/audio/shinespark/shinespark_start.wav",
            charge_loop_sound="assets/audio/shinespark/shinespark_loop.wav",
            charge_fire_sounds=[
                "assets/audio/shinespark/shinespark_stop.wav",
            ],
            charge_time_ms=800,
        ),
    )

    session.add_behavior(
        bank="Tools",
        weapon_id="chainsaw",
        behavior=director.build_chainsaw(
            weapon="chainsaw",
            idle_sound="assets/audio/chainsaw/23_sound_chainsawIdle.wav",
            working_sound="assets/audio/chainsaw/26_sound_chainsawWorking.wav",
            end_sound="assets/audio/chainsaw/24_sound_chainsawEnd.wav",
            end_duration_ms=800,
        ),
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="anakin_hurt",
        behavior=director.build_random_semi_auto(
            weapon="anakin_hurt",
            fire_sounds=[
                "assets/audio/anakin_hurt/anakin_hurt0.wav",
                "assets/audio/anakin_hurt/anakin_hurt1.wav",
                "assets/audio/anakin_hurt/anakin_hurt2.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/anakin_hurt/anakin_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="padme_hurt",
        behavior=director.build_random_semi_auto(
            weapon="padme_hurt",
            fire_sounds=[
                "assets/audio/padme_hurt/padme_hurt0.wav",
                "assets/audio/padme_hurt/padme_hurt1.wav",
                "assets/audio/padme_hurt/padme_hurt2.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/padme_hurt/padme_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="c3p0_hurt",
        behavior=director.build_random_semi_auto(
            weapon="c3p0_hurt",
            fire_sounds=[
                "assets/audio/c3po_hurt/c3po_hurt0.wav",
                "assets/audio/c3po_hurt/c3po_hurt1.wav",
                "assets/audio/c3po_hurt/c3po_hurt2.wav",
                "assets/audio/c3po_hurt/c3po_hurt3.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/c3po_hurt/c3po_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="maul_hurt",
        behavior=director.build_random_semi_auto(
            weapon="maul_hurt",
            fire_sounds=[
                "assets/audio/maul_hurt/MaulHurt1.wav",
                "assets/audio/maul_hurt/MaulHurt2.wav",
                "assets/audio/maul_hurt/MaulHurt3.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/maul_hurt/MaulDeath.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="obiwan_hurt",
        behavior=director.build_random_semi_auto(
            weapon="obiwan_hurt",
            fire_sounds=[
                "assets/audio/obiwan_hurt/obiwan_hurt0.wav",
                "assets/audio/obiwan_hurt/obiwan_hurt1.wav",
                "assets/audio/obiwan_hurt/obiwan_hurt2.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/obiwan_hurt/obiwan_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="ani_hurt",
        behavior=director.build_random_semi_auto(
            weapon="ani_hurt",
            fire_sounds=[
                "assets/audio/ani_hurt/ani_hurt0.wav",
                "assets/audio/ani_hurt/ani_hurt1.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/ani_hurt/ani_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="jarjar_hurt",
        behavior=director.build_random_semi_auto(
            weapon="jarjar_hurt",
            fire_sounds=[
                "assets/audio/jarjar_hurt/jarjar_hurt0.wav",
                "assets/audio/jarjar_hurt/jarjar_hurt1.wav",
                "assets/audio/jarjar_hurt/jarjar_hurt2.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/jarjar_hurt/jarjar_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="wookie_hurt",
        behavior=director.build_random_semi_auto(
            weapon="wookie_hurt",
            fire_sounds=[
                "assets/audio/wookie_hurt/wookie_hurt0.wav",
                "assets/audio/wookie_hurt/wookie_hurt1.wav",
                "assets/audio/wookie_hurt/wookie_hurt2.wav",
                "assets/audio/wookie_hurt/wookie_hurt3.wav",
            ],
            reload_sound="assets/audio/lego_util/lego-star-wars-heart-sound.wav",
            empty_sound="assets/audio/wookie_hurt/wookie_die.wav",
            equip_sound="assets/audio/lego_util/lego-star-wars-switch-character-sound.wav",
        )
    )

    session.add_behavior(
        bank="Characters",
        weapon_id="true_jedi",
        behavior=director.build_charge_shot(
            weapon="true_jedi",
            magazine_size=0,
            charge_start_sound="assets/audio/true_jedi/lego_true_jedi_loop.wav",
            charge_loop_sound="assets/audio/true_jedi/lego_true_jedi_loop.wav",
            charge_fire_sounds=[
                "assets/audio/true_jedi/lego_true_jedi_end.wav",
            ],
            # reload_sound="assets/audio/charge_cannon/reload.wav",
            charge_time_ms=800,
        ),
    )


    session.write_all()


if __name__ == "__main__":
    main()
