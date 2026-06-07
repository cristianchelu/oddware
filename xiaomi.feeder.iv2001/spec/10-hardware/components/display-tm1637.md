# TM1637 — 7-segment LED driver

## Summary

Clock + data serial interface LED driver/controller. Drives the front-panel
readout: three 7-segment digits plus separate status icons and a bi-color
bottom status bar, multiplexed through the TM1637. The IC is inside the front
LCD module plastic, not on the controller PCB (`[probe]` — not desoldered).
See [../pcb.md](../pcb.md). Bit-banged from the SoC on
GPIO1 (DIO) and GPIO13 (CLK). Display rail power is switched via AW9523B P0.5
(active high); segment and icon data are **not** sent over the AW9523B I²C
LED matrix.

## Interface to host


| Parameter    | Value                                      | Source           |
| ------------ | ------------------------------------------ | ---------------- |
| DIO          | MT7682 GPIO1                               | `[probe]`        |
| CLK          | MT7682 GPIO13                              | `[probe]`        |
| Protocol     | 2-wire serial (I²C-like, not standard I²C) | `[ds:TM1637 §7]` |
| Power rail   | AW9523B P0.5 (`mask 0x0020`), active high  | `[probe]`        |
| Power settle | ~100 ms after rail on before TM1637 init   | `[probe]`        |
| VCC          | 3.3–5.5 V                                  | `[ds:TM1637 §5]` |
| Key scan     | Not used (key pins unconnected)            | `[probe]`        |


GPIO1 and GPIO13 must be in GPIO pinmux mode before bit-bang. CLK requires
explicit pull-up register configuration on MT7682 (pin 13); DIO pull-up via
standard GPIO pull-up is sufficient.

## TM1637 command bytes (IV2001)


| Byte          | Datasheet role                                       | On this panel                                      |
| ------------- | ---------------------------------------------------- | -------------------------------------------------- |
| `0x40`        | Data command: write mode, **auto-increment** address | Used before multi-byte burst writes                |
| `0x44`        | Data command: write mode, **fixed** address          | **Preferred** — one grid per transaction `[probe]` |
| `0x80`        | Display off                                          | Power-save / blank `[probe]`                       |
| `0x88`–`0x8B` | Display on, brightness levels 1–4                    | **Four** visible steps on IV2001 `[probe]`         |
| `0x8C`–`0x8F` | Display on, brightness 5–8 (datasheet)               | No visible change above `0x8B` `[probe]`           |
| `0xC0`–`0xC5` | Address command: start at grid 0–5                   | Per-grid write uses `0xC0 | grid` `[probe]`        |


Default brightness on validated hardware: level **4** (`0x8B`).

## On-wire protocol (validated)

### Bit transfer

- Eight data bits per byte, **LSB first**.
- Half-period delay: datasheet minimum ~1 µs; **10 µs** per half-cycle is
reliable on IV2001 `[probe]`.
- **No ACK clock** after data bytes on this panel — eight CLK edges per byte
only (differs from datasheet 9th-bit ACK slot) `[probe]`.

### START condition

1. DIO high, CLK high
2. DIO low (while CLK high)
3. CLK low

### STOP condition (validated tail)

Differs from the textbook “DIO rises while CLK high” STOP alone:

1. DIO low, CLK high
2. Delay
3. DIO high
4. CLK low

Used after each command and per-grid data transaction `[probe]`.

### Recommended refresh sequence

Per-grid **fixed-address** writes avoid byte-alignment ghosts on this PCB:

```text
for grid in 0..4:
  command 0x44
  START → (0xC0 | grid) → segment_byte → STOP
write grid 5 with segment 0x00
command brightness   // 0x88–0x8B on IV2001
```

**Do not** prepend a pad byte or start the burst at a shifted address — that
routes digit bytes onto icon grids `[probe]`.

### Auto-increment burst (optional)

If used: `0x40`, then START → `0xC0` → **five** segment bytes (grids 0–4)
→ STOP, then brightness command. Still clear grid 5 to `0x00` each refresh.
Per-grid `0x44` writes are preferred `[probe]`.

## Grid map (6 COM lines, 5 payload bytes)


| Grid | Frame index | Role                                                                       |
| ---- | ----------- | -------------------------------------------------------------------------- |
| 0    | 0           | Hundreds digit segment pattern                                             |
| 1    | 1           | Tens digit                                                                 |
| 2    | 2           | Ones digit                                                                 |
| 3    | 3           | Pictographs (top row, unit symbols, bottom-row icons)                      |
| 4    | 4           | Bottom-row pictograph (one bit) + status lightbar                          |
| 5    | —           | **Unused** — write `0x00` every refresh (stale RAM ghosts icons) `[probe]` |


## Panel layout

Front panel as viewed by the user (product orientation). Each labeled element
maps to one TM1637 grid and one bit in that grid’s segment byte, except the
three digits (one byte per grid 0–2).

```text
[Child lock]  [Wi‑Fi]  [Dispensing]  ← top row
   [H][T][O]  [%] [g]                ← middle (digits + units)
[Blockage] [! food] [Bowl error]     ← bottom row
  ═══════ status lightbar ═══════    ← orange / green
```


| Row    | Left → right             | Grid | Bit            | Provenance                                           |
| ------ | ------------------------ | ---- | -------------- | ---------------------------------------------------- |
| Top    | Child lock               | 3    | `0x01`         | `[probe]`                                            |
| Top    | Wi‑Fi                    | 3    | `0x02`         | `[probe]`                                            |
| Top    | Dispensing               | 3    | `0x04`         | `[probe]`                                            |
| Middle | Hundreds digit           | 0    | (segment byte) | `[probe]`                                            |
| Middle | Tens digit               | 1    | (segment byte) | `[probe]`                                            |
| Middle | Ones digit               | 2    | (segment byte) | `[probe]`                                            |
| Middle | Percent (`%`)            | 3    | `0x08`         | `[probe]`                                            |
| Middle | Gram (`g`)               | 3    | `0x10`         | `[probe]`                                            |
| Bottom | Blockage                 | 3    | `0x20`         | `[probe]` bit lights an icon; glyph name `[product]` |
| Bottom | Insufficient food (`!`)  | 3    | `0x40`         | `[probe]` bit lights an icon; glyph name `[product]` |
| Bottom | Food bowl error          | 4    | `0x04`         | `[probe]` bit lights an icon; glyph name `[product]` |
| Below  | Status lightbar (orange) | 4    | `0x01`         | `[probe]`                                            |
| Below  | Status lightbar (green)  | 4    | `0x02`         | `[probe]`                                            |


Grid 4 bits `0x01` / `0x02` light the **full-width** orange and green status bar;

## Segment bit wiring

On this PCB, digit segment data uses **standard gfedcba** order (bit 0 = segment
A, top; bits 1–5 = B–F clockwise; bit 6 = G middle). There is no decimal-point
segment on the three digit positions `[probe]`.


| Bit | Segment |
| --- | ------- |
| 0   | A       |
| 1   | B       |
| 2   | C       |
| 3   | D       |
| 4   | E       |
| 5   | F       |
| 6   | G       |


## Numeric digits (0–9)

Digits 0–999 use grids 0–2 with leading-zero suppression on hundreds and tens.
Segment patterns per digit `[probe]`:


| Digit | Segment byte | Segments lit |
| ----- | ------------ | ------------ |
| 0     | `0x3F`       | ABCDEF       |
| 1     | `0x06`       | BC           |
| 2     | `0x5B`       | ABGED        |
| 3     | `0x4F`       | ABCDG        |
| 4     | `0x66`       | BCFG         |
| 5     | `0x6D`       | AFCDG        |
| 6     | `0x7D`       | AFEDCG       |
| 7     | `0x07`       | ABC          |
| 8     | `0x7F`       | ABCDEFG      |
| 9     | `0x6F`       | ABCDFG       |


## Pictograph and lightbar bits (by grid)

Set a bit to `1` to light that element; clear to off. Multiple bits may be on
at once `[probe]`. See [Panel layout](#panel-layout) for physical positions.


| Grid | Bit    | Element                                                  |
| ---- | ------ | -------------------------------------------------------- |
| 3    | `0x01` | Child lock (top row, left)                               |
| 3    | `0x02` | Wi‑Fi (top row, center)                                  |
| 3    | `0x04` | Dispensing (top row, right)                              |
| 3    | `0x08` | Percent (middle row, unit)                               |
| 3    | `0x10` | Gram (middle row, unit)                                  |
| 3    | `0x20` | Bottom-row icon, left (`[product]`: blockage)            |
| 3    | `0x40` | Bottom-row icon, center (`[product]`: insufficient food) |
| 4    | `0x04` | Bottom-row icon, right (`[product]`: food bowl error)    |
| 4    | `0x01` | Status lightbar — orange                                 |
| 4    | `0x02` | Status lightbar — green                                  |


## Deviations from public TM1637 datasheet


| Topic      | Datasheet                      | IV2001 (validated)                                         |
| ---------- | ------------------------------ | ---------------------------------------------------------- |
| ACK        | 9th clock, slave pulls DIO low | **Not used** — 8 data bits only `[probe]`                  |
| STOP       | DIO ↑ while CLK high           | Extended tail: DIO↓, CLK↑, DIO↑, CLK↓ `[probe]`            |
| Brightness | 8 levels `0x88`–`0x8F`         | **4** visible levels `0x88`–`0x8B` `[probe]`               |
| Host I/O   | Open-drain + pull-up           | Push-pull GPIO with pull-ups on CLK (and DIO) `[probe]`    |
| Grids      | Up to 6×8 segments             | Five payload bytes (grids 0–4); **clear grid 5** `[probe]` |


## Application notes

- Power rail (AW9523B P0.5) may be switched off when the panel is not needed;
re-init TM1637 after rail returns with settle delay.
- Icons are bits in grids 3–4 only — not routed through AW9523B segment
registers.

