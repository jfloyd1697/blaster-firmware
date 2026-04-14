from dataclasses import dataclass
from typing import List, Optional, Tuple

from .builder import (
    BehaviorBuilder,
    consume_ammo,
    emit_shot,
    flash_light,
    flash_light_pattern,
    flash_muzzle,
    play_sound,
    play_sound_random,
    pulse_light,
    run_sequence,
    schedule_event,
    set_light,
    solid_light,
    stop_sound,
)
from .model import WeaponBehavior


Color = Tuple[int, int, int]


@dataclass
class CommonLights:
    idle: Color = (0, 120, 255)
    firing: Color = (255, 160, 60)
    empty: Color = (255, 0, 0)
    reload: Color = (0, 255, 80)


class WeaponDirector:
    def __init__(self, lights: Optional[CommonLights] = None) -> None:
        self.lights = lights or CommonLights()

    def build_single_shot(
        self,
        weapon: str,
        magazine_size: int,
        fire_sounds: List[str],
        reload_sound: Optional[str | list[str]] = None,
    ) -> WeaponBehavior:
        builder = BehaviorBuilder(weapon=weapon, magazine_size=magazine_size, initial_state="idle")

        builder.sequence(
            "fire_single",
            [
                play_sound_random(fire_sounds),
                emit_shot(),
                flash_muzzle(),
                consume_ammo(1),
            ],
        )

        if reload_sound:
            if isinstance(reload_sound, str):
                reload_sound = [reload_sound]

            reload_sequence = []
            for reload in reload_sound:
                reload_sequence.extend([
                    play_sound(reload),
                    set_light(pulse_light(self.lights.reload, 180, 180)),
                ])
            builder.sequence("reload_fx", reload_sequence)

        builder.state("idle") \
            .on_enter(set_light(solid_light(self.lights.idle, 120))) \
            .transition("trigger_pressed", "idle", [run_sequence("fire_single")]) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("empty") \
            .on_enter(set_light(pulse_light(self.lights.empty, 180, 220))) \
            .transition("reload_complete", "idle", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("reloading") \
            .transition("reload_complete", "idle", []) \
            .done()

        return builder.build()

    def build_burst(
        self,
        weapon: str,
        magazine_size: int,
        fire_sounds: List[str],
        reload_sound: Optional[str] = None,
        burst_count: int = 3,
        step_delay_ms: int = 80,
    ) -> WeaponBehavior:
        if burst_count < 1:
            raise ValueError("burst_count must be >= 1")

        builder = BehaviorBuilder(weapon=weapon, magazine_size=magazine_size, initial_state="idle")

        builder.sequence(
            "burst_shot",
            [
                play_sound_random(fire_sounds),
                emit_shot(),
                flash_muzzle(),
                consume_ammo(1),
            ],
        )

        if reload_sound:
            builder.sequence(
                "reload_fx",
                [
                    play_sound(reload_sound),
                    set_light(pulse_light(self.lights.reload, 180, 180)),
                ],
            )

        builder.state("idle") \
            .on_enter(set_light(solid_light(self.lights.idle, 120))) \
            .transition(
                "trigger_pressed",
                "burst_1",
                [run_sequence("burst_shot")] + (
                    [schedule_event("burst_step_2", step_delay_ms)] if burst_count > 1 else []
                ),
            ) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        for shot_index in range(1, burst_count):
            state_name = "burst_{0}".format(shot_index)
            next_state = "burst_{0}".format(shot_index + 1)
            event_name = "burst_step_{0}".format(shot_index + 1)
            actions = [run_sequence("burst_shot")]
            if shot_index + 1 < burst_count:
                actions.append(schedule_event("burst_step_{0}".format(shot_index + 2), step_delay_ms))

            builder.state(state_name) \
                .on_enter(set_light(solid_light(self.lights.firing, 220))) \
                .transition(event_name, next_state, actions) \
                .transition("ammo_empty", "empty", []) \
                .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
                .done()

        builder.state("burst_{0}".format(burst_count)) \
            .on_enter(set_light(solid_light(self.lights.firing, 220))) \
            .transition("burst_done", "idle", []) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        # Auto-return after final shot.
        builder.state("burst_{0}".format(burst_count)).transition(
            "burst_step_final",
            "idle",
            [],
        )

        # Simpler explicit final step event.
        if burst_count > 1:
            final_state = builder.behavior.states["burst_{0}".format(burst_count - 1)]
            final_transition = final_state.transitions[0]
            final_transition.target = "idle"
        
        builder.state("empty") \
            .on_enter(set_light(pulse_light(self.lights.empty, 180, 220))) \
            .transition("reload_complete", "idle", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("reloading") \
            .transition("reload_complete", "idle", []) \
            .done()

        return builder.build()

    def build_full_auto(
        self,
        weapon: str,
        magazine_size: int,
        fire_sounds: List[str],
        reload_sound: Optional[str] = None,
        fire_interval_ms: int = 60,
    ) -> WeaponBehavior:
        builder = BehaviorBuilder(weapon=weapon, magazine_size=magazine_size, initial_state="idle")

        builder.sequence(
            "auto_shot",
            [
                play_sound_random(fire_sounds),
                emit_shot(),
                flash_muzzle(),
                consume_ammo(1),
            ],
        )

        if reload_sound:
            builder.sequence(
                "reload_fx",
                [
                    play_sound(reload_sound),
                    set_light(pulse_light(self.lights.reload, 180, 180)),
                ],
            )

        builder.state("idle") \
            .on_enter(set_light(solid_light(self.lights.idle, 120))) \
            .transition(
                "trigger_pressed",
                "auto_firing",
                [run_sequence("auto_shot"), schedule_event("auto_step", fire_interval_ms)],
            ) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("auto_firing") \
            .on_enter(set_light(solid_light(self.lights.firing, 220))) \
            .transition("auto_step", "auto_wait", []) \
            .transition("trigger_released", "idle", []) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("auto_wait") \
            .transition(
                "trigger_held",
                "auto_firing",
                [run_sequence("auto_shot"), schedule_event("auto_step", fire_interval_ms)],
            ) \
            .transition("trigger_released", "idle", []) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("empty") \
            .on_enter(set_light(pulse_light(self.lights.empty, 180, 220))) \
            .transition("reload_complete", "idle", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("reloading") \
            .transition("reload_complete", "idle", []) \
            .done()

        return builder.build()

    def build_charge_shot(
        self,
        weapon: str,
        magazine_size: int,
        charge_start_sound: str,
        charge_loop_sound: str,
        charge_fire_sounds: List[str],
        charge_cancel_sound: Optional[str] = None,
        charge_complete_sound: Optional[str] = None,
        reload_sound: Optional[str] = None,
        charge_time_ms: int = 800,
    ) -> WeaponBehavior:
        builder = BehaviorBuilder(weapon=weapon, magazine_size=magazine_size, initial_state="idle")

        builder.sequence("charge_start", [play_sound(charge_start_sound)])
        builder.sequence("charge_loop", [play_sound(charge_loop_sound, loop=True)])
        builder.sequence(
            "charge_fire",
            [
                stop_sound(),
                play_sound_random(charge_fire_sounds),
                emit_shot(),
                flash_muzzle(),
                consume_ammo(1),
            ],
        )

        if charge_cancel_sound:
            builder.sequence("charge_cancel", [stop_sound(), play_sound(charge_cancel_sound)])
        if charge_complete_sound:
            builder.sequence("charge_complete_fx", [play_sound(charge_complete_sound)])
        if reload_sound:
            builder.sequence(
                "reload_fx",
                [
                    play_sound(reload_sound),
                    set_light(pulse_light(self.lights.reload, 180, 180)),
                ],
            )

        builder.state("idle") \
            .on_enter(set_light(solid_light(self.lights.idle, 120))) \
            .transition(
                "trigger_pressed",
                "charging",
                [run_sequence("charge_start"), run_sequence("charge_loop"), schedule_event("charge_complete", charge_time_ms)],
            ) \
            .transition("ammo_empty", "empty", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("charging") \
            .on_enter(set_light(pulse_light(self.lights.firing, 180, 120))) \
            .transition(
                "charge_complete",
                "charged",
                [run_sequence("charge_complete_fx")] if charge_complete_sound else [],
            ) \
            .transition(
                "trigger_released",
                "idle",
                [run_sequence("charge_cancel")] if charge_cancel_sound else [stop_sound()],
            ) \
            .transition("reload", "reloading", [stop_sound()] + ([run_sequence("reload_fx")] if reload_sound else [])) \
            .done()

        builder.state("charged") \
            .on_enter(set_light(solid_light(self.lights.firing, 255))) \
            .transition("trigger_released", "idle", [run_sequence("charge_fire")]) \
            .transition("reload", "reloading", [stop_sound()] + ([run_sequence("reload_fx")] if reload_sound else [])) \
            .done()

        builder.state("empty") \
            .on_enter(set_light(pulse_light(self.lights.empty, 180, 220))) \
            .transition("reload_complete", "idle", []) \
            .transition("reload", "reloading", [run_sequence("reload_fx")] if reload_sound else []) \
            .done()

        builder.state("reloading") \
            .transition("reload_complete", "idle", []) \
            .done()

        return builder.build()

    def build_chainsaw(
        self,
        weapon: str,
        idle_sound: str,
        working_sound: str,
        end_sound: str,
        end_duration_ms: int = 800,
    ) -> WeaponBehavior:
        builder = BehaviorBuilder(weapon=weapon, magazine_size=1, initial_state="idle")

        builder.sequence("idle_sound", [play_sound(idle_sound, loop=True)])
        builder.sequence("working_sound", [play_sound(working_sound, loop=True)])
        builder.sequence("end_sound", [play_sound(end_sound)])

        builder.state("idle") \
            .on_enter(
                set_light(solid_light((255, 160, 0), 180)),
                run_sequence("idle_sound"),
            ) \
            .on_exit(stop_sound()) \
            .transition("trigger_pressed", "working", [run_sequence("working_sound")]) \
            .done()

        builder.state("working") \
            .on_enter(set_light(solid_light((255, 120, 0), 220))) \
            .on_exit(stop_sound()) \
            .transition("trigger_released", "ending", [run_sequence("end_sound")]) \
            .done()

        builder.state("ending") \
            .on_enter(
                set_light(solid_light((80, 0, 0), 30)),
                schedule_event("return_to_idle", end_duration_ms),
            ) \
            .transition("return_to_idle", "idle", []) \
            .transition("trigger_pressed", "working", [run_sequence("working_sound")]) \
            .done()

        return builder.build()

    def build_random_semi_auto(
        self,
        weapon: str,
        fire_sounds: List[str],
        reload_sound: str = None,
        empty_sound: str = None,
        equip_sound: str = None,
        num_shots: int = None,
    ) -> WeaponBehavior:
        if num_shots is None:
            num_shots = len(fire_sounds)

        if empty_sound is not None:
            num_shots += 1

        builder = BehaviorBuilder(weapon=weapon, magazine_size=num_shots, initial_state=f"equip")

        equip_sequence = [
                set_light(solid_light((0, 255, 255), 120)),
            ]
        if equip_sound:
            equip_sequence.insert(0, play_sound(equip_sound))

        equip_sequence.append(schedule_event(f"shot_{num_shots}", 500))
        builder.sequence("equip", equip_sequence)

        builder.sequence(
            "fire_sounds",
            [
                play_sound_random(fire_sounds),
                flash_muzzle(),
                flash_light(flash_light_pattern((255, 80, 0), 255, 80, 1)),
                consume_ammo(1),
            ],
        )

        if empty_sound is not None:
            builder.sequence(
                "empty_sound",
                [
                    play_sound(empty_sound),
                    flash_muzzle(),
                    set_light(solid_light((255, 0, 0), 40)),
                    consume_ammo(1),
                ],
            )

        reload_sequence = [set_light(pulse_light((0, 255, 80), 200, 160))]
        if reload_sound:
            reload_sequence.insert(0, play_sound(reload_sound))

        builder.sequence("reload_fx", reload_sequence)

        shots = [(f"shots_{i}", "fire_sounds") for i in range(num_shots, 0, -1)]
        empty_state = ("empty", "empty_sound") if empty_sound else shots[-1]
        shots[-1] = empty_state

        for i, (state, sound) in enumerate(shots):
            if i + 1 < len(shots):
                next_state, next_sound = shots[i+1]
            else:
                next_state, next_sound = empty_state

            builder.state(state) \
            .on_enter(set_light(solid_light((255, 160 - i * 40, 0), 180))) \
            .transition("trigger_pressed", next_state, [run_sequence(next_sound)]) \
            .transition("reload", "reloading", [run_sequence("reload_fx")]) \
            .done()

        builder.state("reloading") \
            .transition("reload_complete", shots[0][0], []) \
            .done()

        return builder.build()
