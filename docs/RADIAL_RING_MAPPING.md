# Diagnosing a radial pulse that looks like a chase

The `cpx_ring_61` coordinate layout assumes LED indices are wired by concentric ring:

| Stage | Expected physical ring | LED indices | Color |
|---|---|---|---|
| 1 | Center | 0 | Red |
| 2 | Inner ring | 1–8 | Yellow |
| 3 | Second ring | 9–20 | Green |
| 4 | Third ring | 21–36 | Blue |
| 5 | Outer ring | 37–60 | Purple |

The **ring identification** file illuminates one of those five groups at a time, for
600 ms per group, in a 3-second compiled sequence. It is a diagnostic of index-to-position
mapping, not a replacement for the normal radial animation.

## No firmware reflash required

1. Back up `assets/lights/diagnostics/radial_pulse.bin` on your SD card.
2. Copy `assets/lights/diagnostics/ring_identification.bin` onto the card under the
   filename `assets/lights/diagnostics/radial_pulse.bin`.
3. Run diagnostic option **0**. Its second effect will show the five ring groups in
   sequence; the showcase continues to loop until `q`.
4. Observe whether all LEDs in each illuminated group belong to the *same* physical
   ring. If a group appears scattered or runs along a winding path, the layout's
   index mapping does not match the actual wiring; changing the brightness envelope
   cannot fix that. Record the actual pixel indices for each ring.
5. Restore the original `radial_pulse.bin` (which is now an expanding **filled disc**
   and then contracting disc, not a thin moving band).

The physical wiring order cannot be derived from a coordinate-only JSON layout.
Do not infer a new layout mapping without this verification.

Regenerate either file on your host without extra Python dependencies:

```sh
python tools/generate_light_showcase.py --effect radial_pulse
python tools/generate_light_showcase.py --effect ring_identification
```
