import json
from typing import Any, Dict, List, Optional, Tuple

from .model import Action, State, Transition, WeaponBehavior


Color = Tuple[int, int, int]


def solid_light(color: Color, brightness: int) -> Dict[str, Any]:
    return {
        "mode": "solid",
        "color": list(color),
        "brightness": brightness,
    }


def pulse_light(color: Color, brightness: int, interval_ms: int) -> Dict[str, Any]:
    return {
        "mode": "pulse",
        "color": list(color),
        "brightness": brightness,
        "intervalMs": interval_ms,
    }


def flash_light_pattern(
    color: Color,
    brightness: int,
    duration_ms: int,
    count: int = 1,
) -> Dict[str, Any]:
    return {
        "mode": "flash",
        "color": list(color),
        "brightness": brightness,
        "durationMs": duration_ms,
        "count": count,
    }


class BehaviorBuilder:
    def __init__(self, weapon: str, magazine_size: int, initial_state: str) -> None:
        self.behavior = WeaponBehavior(
            weapon=weapon,
            magazine_size=magazine_size,
            initial_state=initial_state,
        )

    def sequence(self, name: str, actions: List[Action]) -> "BehaviorBuilder":
        self.behavior.action_sequences[name] = actions
        return self

    def state(self, name: str) -> "StateBuilder":
        state = self.behavior.states.get(name)
        if state is None:
            state = State(name=name)
            self.behavior.states[name] = state
        return StateBuilder(self, state)

    def build(self) -> WeaponBehavior:
        return self.behavior

    def to_dict(self) -> Dict[str, Any]:
        return self.behavior.to_dict()

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


class StateBuilder:
    def __init__(self, parent: BehaviorBuilder, state: State) -> None:
        self.parent = parent
        self.state = state

    def on_enter(self, *actions: Action) -> "StateBuilder":
        self.state.on_enter.extend(actions)
        return self

    def on_exit(self, *actions: Action) -> "StateBuilder":
        self.state.on_exit.extend(actions)
        return self

    def transition(
        self,
        event: str,
        target: str,
        actions: Optional[List[Action]] = None,
    ) -> "StateBuilder":
        self.state.transitions.append(
            Transition(
                event=event,
                target=target,
                actions=actions or [],
            )
        )
        return self

    def done(self) -> BehaviorBuilder:
        return self.parent


# Action helpers

def play_sound(sound: str, loop: bool = False) -> Action:
    params: Dict[str, Any] = {"sound": sound}
    if loop:
        params["loop"] = True
    return Action("play_sound", params)


def play_sound_random(sounds: List[str], loop: bool = False) -> Action:
    params: Dict[str, Any] = {"sounds": sounds}
    if loop:
        params["loop"] = True
    return Action("play_sound_random", params)


def stop_sound() -> Action:
    return Action("stop_sound")


def set_light(pattern: Dict[str, Any]) -> Action:
    return Action("set_light", {"pattern": pattern})


def flash_light(pattern: Dict[str, Any]) -> Action:
    return Action("flash_light", {"pattern": pattern})


def emit_shot() -> Action:
    return Action("emit_shot")


def flash_muzzle() -> Action:
    return Action("flash_muzzle")


def consume_ammo(amount: int = 1) -> Action:
    return Action("consume_ammo", {"amount": amount})


def emit_event(event: str) -> Action:
    return Action("emit_event", {"event": event})


def schedule_event(event: str, delay_ms: int) -> Action:
    return Action("schedule_event", {"event": event, "delayMs": delay_ms})


def run_sequence(name: str) -> Action:
    return Action("run_sequence", {"name": name})
