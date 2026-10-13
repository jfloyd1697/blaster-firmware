"""Generate SD-ready WFLSEQ01 demos for all six editor light-layer types.

This standalone generator uses the CPX 61-LED layout geometry and produces
precompiled frames for the firmware's existing bounded-memory streaming player.
It has no third-party dependencies and never runs on the microcontroller.

The effect calculations are matched to the six layer types in the editor's
weapon_fsm_lights.domain.compiler. Regenerate after changing the layout.
"""

from __future__ import annotations

import argparse
import json
import math
import random
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAYOUT = ROOT / "tools/light_showcase/cpx_ring_61.json"
OUTPUT = ROOT / "sdcard/assets/lights/diagnostics"
LED_COUNT = 61
FRAME_MS = 50
FRAME_COUNT = 60
TYPES = ("solid", "radial_pulse", "wipe", "blink", "chase", "sparkle")


def radial_pulse_intensity(x: float, y: float, frame: int, outer_radius: float) -> float:
    """Fill concentric rings center-out and fade outer-in without a moving dot.

    Every LED at the same radius gets the same alpha. The leading edge is
    feathered, while already-lit inner rings stay lit until contraction.
    """
    progress = frame / FRAME_COUNT
    reach = (1.0 - abs(2.0 * progress - 1.0)) * (outer_radius + 0.06)
    distance = math.hypot(x - 0.5, y - 0.5)
    edge_width = 0.065
    edge = (reach - distance + edge_width / 2.0) / edge_width
    edge = max(0.0, min(1.0, edge))
    # Smoothstep suppresses sharp flashes between adjacent rings.
    return 0.62 * edge * edge * (3.0 - 2.0 * edge)


RING_RANGES = ((0, 1), (1, 9), (9, 21), (21, 37), (37, 61))
RING_COLORS = (
    (255, 0, 0),
    (255, 180, 0),
    (0, 255, 0),
    (0, 60, 255),
    (180, 0, 255),
)


def ring_identification_pixel(index: int, frame: int) -> tuple[tuple[int, int, int], float]:
    """Five static index groups, 600 ms per group, one group at a time."""
    group = (frame * len(RING_RANGES)) // FRAME_COUNT
    start, stop = RING_RANGES[group]
    return RING_COLORS[group], 0.5 if start <= index < stop else 0.0


def wipe_intensity(x: float, frame: int) -> float:
    """Left-to-right filled wipe, not a traveling bright stripe.

    Previously illuminated LEDs remain on until the stage ends. The front
    moves across the entire layout (with a narrow smooth leading edge).
    """
    progress = frame / (FRAME_COUNT - 1)
    front = -0.065 + 1.13 * progress
    edge_width = 0.085
    coverage = (front - x + edge_width / 2.0) / edge_width
    coverage = max(0.0, min(1.0, coverage))
    return 0.42 * coverage * coverage * (3.0 - 2.0 * coverage)


def intensity_for(
    name: str, index: int, x: float, y: float, frame: int, outer_radius: float
) -> tuple[tuple[int, int, int], float]:
    progress = frame / FRAME_COUNT
    if name == "solid":
        return (0, 180, 255), 0.22
    if name == "radial_pulse":
        return (255, 145, 12), radial_pulse_intensity(x, y, frame, outer_radius)
    if name == "ring_identification":
        return ring_identification_pixel(index, frame)
    if name == "wipe":
        return (45, 255, 65), wipe_intensity(x, frame)
    if name == "blink":
        phase = (progress * 6) % 1.0
        return (190, 40, 255), 0.25 if phase < 0.5 else 0.0
    if name == "chase":
        # The editor chase layer uses index/1000 as its ring-coordinate.
        # Limit the sweep to the 61 LED indices, rather than all 1000 slots.
        position = (progress * 0.061) % 1.0
        led_position = (index % 1000) / 1000.0
        distance = abs(led_position - position)
        distance = min(distance, 1.0 - distance)
        strength = max(0.0, 1.0 - distance / 0.007)
        return (25, 95, 255), strength * 0.6
    if name == "sparkle":
        # Matches the editor's deterministic seed/index/time-bucket method.
        bucket = frame  # time_ms // int(100/speed), speed=2, frame_ms=50
        rng = random.Random(17 + index * 131 + bucket * 17)
        return (255, 255, 255), 0.36 if rng.random() < 0.15 else 0.0
    raise ValueError(f"Unknown effect type: {name}")


def encode(name: str, leds: list[dict[str, float | int]]) -> bytes:
    total_ms = FRAME_COUNT * FRAME_MS
    outer_radius = max(math.hypot(float(led["u"]) - 0.5, float(led["v"]) - 0.5) for led in leds)
    payload = bytearray(struct.pack("<8sHHII", b"WFLSEQ01", 1, len(leds), FRAME_COUNT, total_ms))
    for frame_index in range(FRAME_COUNT):
        # Include all 61 LEDs, including zero-brightness LEDs, so the sequence
        # explicitly clears previous frames on every update.
        payload.extend(struct.pack("<IH", FRAME_MS, len(leds)))
        for led in leds:
            index = int(led["index"])
            color, intensity = intensity_for(
                name, index, float(led["u"]), float(led["v"]), frame_index, outer_radius
            )
            alpha = max(0, min(255, round(intensity * 255)))
            payload.extend(struct.pack("<HBBBB", index, *color, alpha))
    return bytes(payload)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate compiled 61-LED light diagnostics")
    parser.add_argument(
        "--effect", choices=(*TYPES, "ring_identification"),
        help="Regenerate only one effect (default: all six showcase effects)",
    )
    args = parser.parse_args()

    data = json.loads(LAYOUT.read_text(encoding="utf-8"))
    leds = sorted(data["leds"], key=lambda item: item["index"])
    if data.get("coordinate_space") != "normalized_2d" or [led["index"] for led in leds] != list(range(LED_COUNT)):
        raise ValueError("Expected normalized cpx_ring_61 layout, contiguous LED indices 0..60")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in (args.effect,) if args.effect else TYPES:
        target = OUTPUT / f"{name}.bin"
        encoded = encode(name, leds)
        target.write_bytes(encoded)
        print(f"{target.relative_to(ROOT)}: {len(encoded)} bytes, {FRAME_COUNT} frames")


if __name__ == "__main__":
    main()
