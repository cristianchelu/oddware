# Display driver

serves:
  - ../20-stories/display.md

Low-level TM1637 transport, grid map, and rendering primitives. What to show
and when: [display-presentation.md](display-presentation.md).

## Hardware invariants

Front readout: three 7-segment digits, main/side pictographs, and a bi-color
bottom status bar, all driven by a TM1637 on GPIO bit-bang.

| Signal | Connection |
|--------|------------|
| TM1637 DIO | MT7682 GPIO1 `[probe]` |
| TM1637 CLK | MT7682 GPIO13 `[probe]` |
| Display rail | AW9523B P0.5, active high `[probe]` |
| Rail settle | ~100 ms after P0.5 on before TM1637 traffic `[probe]` |

Authoritative grid map, segment LUT, icon masks, and protocol details:
[display-tm1637.md](../10-hardware/components/display-tm1637.md).

## TM1637 on-wire protocol

Two-wire serial (I²C-like but not standard I²C). Bit-banged on GPIO1/GPIO13.

### Per-grid refresh (validated on IV2001)

```text
for grid in 0..4:
  command 0x44                    // fixed address, write mode
  START → (0xC0 | grid) → seg → STOP
write grid 5 with 0x00            // prevent icon ghosting
command 0x88..0x8B                // brightness 1–4 only on this panel
```

- Data bits: **8 per byte, LSB first**; **no ACK clock** `[probe]`.
- Half-period: ≥ 1 µs (datasheet); **10 µs** reliable on IV2001 `[probe]`.
- STOP tail after each transaction: DIO↓, CLK↑, delay, DIO↑, CLK↓ `[probe]`.
- Do **not** prepend a pad byte or mis-align the burst start — digit bytes
  will land on icon grids `[probe]`.

### Command bytes used

| Byte | Role |
|------|------|
| `0x44` | Fixed-address data write (preferred) |
| `0x40` | Auto-increment data write (optional burst path) |
| `0xC0`–`0xC4` | Grid 0–4 address prefix inside a data transaction |
| `0x80` | Display off |
| `0x88`–`0x8B` | Display on, brightness 1–4 (only four visible steps) |
| `0x8C`–`0x8F` | Datasheet levels 5–8; visually same as `0x8B` on IV2001 |

Key-scan readback is not used (key pins unconnected) `[probe]`.

## Panel segment map

| Element | Grid | Bit / bytes | Notes |
|---------|------|-------------|-------|
| Hundreds digit | 0 | segment byte | Spinner/animation may replace this byte |
| Tens digit | 1 | segment byte | |
| Ones digit | 2 | segment byte | |
| Colon / separator | 3 | `0x01` | |
| Blink phase A | 3 | `0x02` | Auxiliary |
| Dispense pictograph | 3 | `0x04` | |
| Percent pictograph | 3 | `0x08` | |
| Gram pictograph | 3 | `0x10` | |
| Jam (main) | 3 | `0x20` | |
| Underfill (main) | 3 | `0x40` | |
| Yellow status bar | 4 | `0x01` | Full-width bar, not a bowl icon |
| Green status bar | 4 | `0x02` | |
| Jam / blockage (side) | 4 | `0x04` | |
| Grid 5 | 5 | always `0x00` | Cleared each refresh |

Digit segment encoding: bit 0 = A … bit 6 = G. Digit LUT: `3F 06 5B 4F 66 6D
7D 07 7F 6F` for digits 0–9 `[probe]`.

Full tables: [display-tm1637.md](../10-hardware/components/display-tm1637.md).

## Brightness command map

| Command byte | Effect on IV2001 |
|--------------|-------------------|
| `0x80` | Display OFF |
| `0x88` | Brightness 1 (dimmest visible) |
| `0x89` | Level 2 |
| `0x8A` | Level 3 |
| `0x8B` | Level 4 (brightest visible) |
| `0x8C`–`0x8F` | No visible change vs `0x8B` `[probe]` |

## Character set

7-segment patterns for digits 0–9: see hardware spec. Additional on-wire
patterns:

| Pattern | Bytes 0–2 | Usage |
|---------|-----------|-------|
| `0x40` on all three digit grids | Segment dash |
| `0x79`, `0x3F`, `0x06` | Static percent glyph |
| Multi-frame digit animation | Bytes 0–2 vary per frame | Busy / lock spinner |

## Rail power primitives

| Action | Hardware |
|--------|----------|
| Rail on | Set P0.5 high; wait `[tune]` ~100 ms before TM1637 traffic |
| Rail off | Set P0.5 low |

When to invoke rail on/off is defined in
[display-presentation.md](display-presentation.md) and
[power-state-machine.md](power-state-machine.md).
