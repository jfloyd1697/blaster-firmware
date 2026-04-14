import pathlib
from dataclasses import dataclass, field
from typing import Any, Dict, List


@dataclass
class Action:
    type: str
    params: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        data = {"type": self.type}
        data.update(self.params)
        return data

    def files(self) -> List[pathlib.Path]:
        files = []
        if sound := self.params.get("sound"):
            files.append(pathlib.Path(sound))
        files.extend([pathlib.Path(sound) for sound in self.params.get("sounds", [])])
        return files



@dataclass
class Transition:
    event: str
    target: str
    actions: List[Action] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        data = {
            "event": self.event,
            "target": self.target,
        }
        if self.actions:
            data["actions"] = [action.to_dict() for action in self.actions]
        return data


@dataclass
class State:
    name: str
    on_enter: List[Action] = field(default_factory=list)
    on_exit: List[Action] = field(default_factory=list)
    transitions: List[Transition] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "onEnter": [action.to_dict() for action in self.on_enter],
            "onExit": [action.to_dict() for action in self.on_exit],
            "transitions": [transition.to_dict() for transition in self.transitions],
        }


@dataclass
class WeaponBehavior:
    weapon: str
    magazine_size: int
    initial_state: str
    action_sequences: Dict[str, List[Action]] = field(default_factory=dict)
    states: Dict[str, State] = field(default_factory=dict)
    version: int = 1

    def to_dict(self) -> Dict[str, Any]:
        return {
            "version": self.version,
            "weapon": self.weapon,
            "magazineSize": self.magazine_size,
            "initialState": self.initial_state,
            "actionSequences": {
                name: [action.to_dict() for action in actions]
                for name, actions in self.action_sequences.items()
            },
            "states": {
                name: state.to_dict()
                for name, state in self.states.items()
            },
        }
