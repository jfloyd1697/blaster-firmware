"""Experimental STM32 USB CDC bridge client. Requires pyserial."""
from __future__ import annotations

import threading
from typing import Literal

import serial


class BridgeClient:
    def __init__(self, port: str, baudrate: int = 115200) -> None:
        self._serial = serial.Serial(port, baudrate, timeout=0.3)
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._heartbeat: threading.Thread | None = None

    def send(self, command: str) -> None:
        if "\n" in command or "\r" in command:
            raise ValueError("Commands must contain one line")
        with self._lock:
            self._serial.write((command + "\n").encode("ascii"))

    def set_mode(self, mode: Literal["NORMAL", "INPUT", "DIRECT"]) -> None:
        if mode not in ("NORMAL", "INPUT", "DIRECT"):
            raise ValueError(mode)
        self.send(f"MODE {mode}")
        if mode != "NORMAL" and self._heartbeat is None:
            self._heartbeat = threading.Thread(target=self._heartbeat_loop, daemon=True)
            self._heartbeat.start()

    def _heartbeat_loop(self) -> None:
        while not self._stop.wait(0.4):
            self.send("PING")

    def input(self, name: Literal["TRIGGER", "RELOAD", "NEXT", "PREVIOUS"], pressed: bool) -> None:
        if name not in ("TRIGGER", "RELOAD", "NEXT", "PREVIOUS"):
            raise ValueError(name)
        self.send(f"INPUT {name} {int(pressed)}")

    def play_audio(self, path: str) -> None:
        self.send(f"AUDIO {path}")

    def play_light(self, path: str) -> None:
        self.send(f"LIGHT {path}")

    def stop_outputs(self) -> None:
        self.send("STOP")

    def read_line(self) -> str:
        return self._serial.readline().decode("utf-8", errors="replace").rstrip()

    def close(self) -> None:
        self._stop.set()
        if self._heartbeat is not None:
            self._heartbeat.join(timeout=1)
        self.send("MODE NORMAL")
        self._serial.close()

    def __enter__(self) -> BridgeClient:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()
