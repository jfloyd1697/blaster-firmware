# 🔫 Blaster Wiring

## 🎧 Audio (I2S – MAX98357A)

```text
NodeMCU   →   MAX98357A   Notes
--------- --- ----------- -------------------------
VU        →   VIN         5V for best audio power
GND       →   GND         Common ground
D4        →   LRC         Word select (WS)
D2        →   BCLK        Bit clock
RX        →   DIN         Data in
```

### ⚠️ Notes

* Keep these wires **VERY short (≤ 2–3 inches)**
* Do NOT use D8 (boot issues)
* RX is used for audio → no serial RX while running

---

## 🔊 Speaker

```text
MAX98357A   →   Speaker
----------- --- ---------
SPK+        →   +
SPK-        →   -
```

✔ Use **twisted pair wire**
✔ Do NOT connect speaker to ground

---

## 💾 SD Card (SPI)

```text
NodeMCU   →   SD Module   Notes
--------- --- ----------- -------------------------
3V3       →   VCC         3.3V ONLY (important)
GND       →   GND         Common ground
D7        →   MOSI        SPI data out
D6        →   MISO        SPI data in
D5        →   SCK         SPI clock
D1        →   CS          Chip select
```

### Notes

* Keep under ~6 inches
* Card must be **FAT32**
* Place file at:

  ```
  /assets/weapon_profiles.json
  ```

---

## 🔘 Buttons (Resistor Ladder on A0)

### 🎯 Purpose

Use **1 pin (A0)** for multiple buttons:

* Next
* Previous
* Reload
* Trigger

---

## 🔌 Ladder Wiring

```text
3V3
 |
[10k]  ← Pull-up
 |
 +------ A0 (NodeMCU)
 |
 +--[Button]--[1k]--- GND      → Next
 |
 +--[Button]--[2.2k]- GND      → Previous
 |
 +--[Button]--[4.7k]- GND      → Reload
```

---

## ⚠️ Important A0 Notes

* NodeMCU A0 is internally scaled → safe with 3.3V input
* Still good practice to:

    * avoid direct shorts
    * use resistors as shown

---

## 🎯 Trigger Button (Separate)

```text
NodeMCU   →   Button
--------- --- --------
D0        →   Button → GND
```

---

## 🧠 Behavior Mapping (final)

| Input         | 	Action          |
|---------------|------------------|    
| Trigger       | 	Fire            |
| Next (short)  | 	Next weapon     |
| Next (long)   | 	Next bank       |
| Prev (short)  | 	Previous weapon |    
| Prev (long)   | 	Previous bank   |    
| Reload        | 	Reload          |

---

### Config

* Use `INPUT_PULLUP`
* D0 is perfect for simple trigger input

---

## ⚡ Power Layout

```text
VU (5V)  → MAX98357A
3V3      → SD card + resistor ladder
GND      → EVERYTHING
```

✔ All grounds must be connected

---

## 🚨 Boot Safety Rules

| Pin         | Must be at boot |
| ----------- | --------------- |
| D3 (GPIO0)  | HIGH            |
| D4 (GPIO2)  | HIGH            |
| D8 (GPIO15) | LOW             |

---

## 🧪 Bring-Up Checklist

Before testing:

* [ ] SD card formatted FAT32
* [ ] `/weapon_profiles.json` exists
* [ ] WAV files present
* [ ] Speaker connected
* [ ] I2S wires short
* [ ] Common ground verified
