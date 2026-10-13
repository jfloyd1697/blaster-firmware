"""Tests for static ring identification and 61-LED radial fill."""

import importlib.util
import json
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR_PATH = ROOT / "tools/generate_light_showcase.py"
SPEC = importlib.util.spec_from_file_location("light_showcase_generator", GENERATOR_PATH)
assert SPEC is not None and SPEC.loader is not None
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)


class RadialDiagnosticTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        data = json.loads(GENERATOR.LAYOUT.read_text(encoding="utf-8"))
        cls.leds = sorted(data["leds"], key=lambda led: led["index"])
        cls.outer = max(
            ((led["u"] - 0.5) ** 2 + (led["v"] - 0.5) ** 2) ** 0.5
            for led in cls.leds
        )
        cls.rings = [range(a, b) for a, b in GENERATOR.RING_RANGES]

    def levels(self, frame):
        return [
            [GENERATOR.radial_pulse_intensity(
                self.leds[i]["u"], self.leds[i]["v"], frame, self.outer
            ) for i in ring]
            for ring in self.rings
        ]

    def test_all_pixels_on_a_ring_have_same_brightness(self):
        for frame in range(60):
            for ring in self.levels(frame):
                self.assertLess(max(ring) - min(ring), 0.00002)

    def test_radial_fill_expands_then_contracts(self):
        for frame in range(60):
            levels = [sum(ring) / len(ring) for ring in self.levels(frame)]
            # Always brighter toward the center; never an isolated traveling band.
            self.assertTrue(all(a + .0001 >= b for a, b in zip(levels, levels[1:])))
        for frame, lit_rings in ((0, 1), (15, 3), (30, 5), (45, 3), (59, 1)):
            levels = [sum(ring) / len(ring) for ring in self.levels(frame)]
            count = sum(level > 0.2 for level in levels)
            self.assertEqual(count, lit_rings, (frame, levels))

    def test_ring_identification_colors_exact_groups(self):
        for frame in range(60):
            group = frame // 12
            start, stop = GENERATOR.RING_RANGES[group]
            for index in range(61):
                color, brightness = GENERATOR.ring_identification_pixel(index, frame)
                self.assertEqual(color, GENERATOR.RING_COLORS[group])
                self.assertEqual(brightness, .5 if start <= index < stop else 0)


    def test_wipe_is_filled_and_never_chases(self):
        """Pixels must stay illuminated once the advancing front passes."""
        for led in self.leds:
            levels = [GENERATOR.wipe_intensity(led["u"], frame) for frame in range(60)]
            self.assertAlmostEqual(levels[0], 0.0)
            self.assertAlmostEqual(levels[-1], 0.42)
            self.assertTrue(all(left <= right + 1e-9 for left, right in zip(levels, levels[1:])))

    def test_wipe_front_is_spatial_not_led_index_order(self):
        for frame in (10, 20, 30, 40, 50):
            levels = [GENERATOR.wipe_intensity(led["u"], frame) for led in self.leds]
            # Increasing X coordinates must never light before smaller X.
            ordered = sorted(zip((led["u"] for led in self.leds), levels))
            self.assertTrue(all(a >= b - 1e-9 for (_, a), (_, b) in zip(ordered, ordered[1:])))
        center = GENERATOR.wipe_intensity(0.5, 30)
        self.assertGreater(center, 0.0)
        self.assertLess(center, 0.42)

    def test_wipe_sd_binary_matches_generator(self):
        payload = (ROOT / "sdcard/assets/lights/diagnostics/wipe.bin").read_bytes()
        self.assertEqual(payload, GENERATOR.encode("wipe", self.leds))
        self.assertEqual(struct.unpack_from("<8sHHII", payload, 0),
                         (b"WFLSEQ01", 1, 61, 60, 3000))

    def test_sd_binaries_match_generator(self):
        for name in ("radial_pulse", "ring_identification"):
            payload = (ROOT / "sdcard/assets/lights/diagnostics" / f"{name}.bin").read_bytes()
            self.assertEqual(payload, GENERATOR.encode(name, self.leds))
            self.assertEqual(
                struct.unpack_from("<8sHHII", payload, 0),
                (b"WFLSEQ01", 1, 61, 60, 3000),
            )
            self.assertEqual(len(payload), 20 + 60 * (6 + 61 * 6))


if __name__ == "__main__":
    unittest.main()
