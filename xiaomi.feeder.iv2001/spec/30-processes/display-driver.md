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

## Software layering

`[design]` This file owns **how** the panel is driven: protocol, grid map,
rail settle, and rendering primitives behind `display_port`.

| Firmware module | Role |
|-----------------|------|
| `tm1637.c` | TM1637 bit-bang protocol (GPIO1/GPIO13 only) |
| `display_rail.c` | Display power rail policy (AW9523B P0.5, settle delay) |
| `display_driver.c` | Composes rail + TM1637; enforces rail-before-segment-data |
| `display_adapter.c` | HAL binding for TM1637 GPIO and expander access |
| `aw9523b.c`, `gpio_expander_adapter.c` | Expander register model and I2C (P0.5 rail) |

**Out of scope here and in those modules:** display modes, MQTT/NVDM policy,
boot UX timing (except hardware rail settle), connectivity blinkers. What to
show and when: [display-presentation.md](display-presentation.md).

**Port surface:** `display_port` primitives (`power_on`, `show_grids`, `blank`,
`set_brightness`). Logical composition lives in `display_glyph.c`;
presentation code is the sole caller of `display_port` for user-visible behavior.

**Physical seam:** AW9523B P0.5 energizes the TM1637 module; segment data uses
SoC GPIO only. Only `display_driver.c` sequences rail-on before TM1637 traffic.

`gpio_expander_bootstrap()` resets all expander outputs to boot defaults
(including P0.5 off). Only display boot / `display_power_on` may call it.
Other consumers (e.g. weigh scale P0.2) use `set_pin` only. After bootstrap,
`display_driver` and `display_presentation` invalidate their powered state so
the next refresh re-asserts P0.5.

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
| Child lock | 3 | `0x01` | |
| Wi‑Fi | 3 | `0x02` | |
| Dispense pictograph | 3 | `0x04` | |
| Percent pictograph | 3 | `0x08` | |
| Gram pictograph | 3 | `0x10` | |
| Blockage | 3 | `0x20` | |
| Insufficient food | 3 | `0x40` | |
| Status lightbar (orange) | 4 | `0x01` | Full-width bar |
| Status lightbar (green) | 4 | `0x02` | |
| Food bowl error | 4 | `0x04` | |
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

## Boot self-test

At early application startup the panel runs a hardware bring-up sequence.
**Invocation:** `display_boot_run()` from patched SDK `system_init()` **before**
`connsys_init()` — see [build-integration.md](../40-architecture/build-integration.md)
§ Display boot before Wi-Fi SPI. GPIO12–16 are shared with the WFCI SPI bus;
after Wi-Fi firmware download those pins are not available for AW9523B I2C.

Timing and segment values below are **presentation policy** (traceability);
firmware implements the policy in `display_boot.c`, not in the driver stack.
See [display-presentation.md](display-presentation.md).

| Step | Layer | Action |
|------|-------|--------|
| 0 | Presentation | `[tune]` **50 ms** settle before rail on (`DISPLAY_BOOT_PRE_POWER_MS`) |
| 1 | Expander port | GPIO14 reset pulse (100 ms), I2C1 init (GPIO15 SCL / GPIO16 SDA), ID `0x10` == `0x23`, CTL GPIO mode |
| 2 | Bootstrap | Safe P0/P1 directions and outputs — motor off, CS1270 off, index LED off, **display rail P0.5 low** |
| 3 | Display driver | TM1637 pin prep (GPIO1/13), then `display_rail_on()` — **P0.5 high** |
| 4 | Display rail | Wait `[tune]` **100 ms** settle (`DISPLAY_RAIL_SETTLE_MS`) |
| 5 | TM1637 | Refresh grids 0–4 = `0xFF`, grid 5 = `0x00`, brightness `0x8B` |
| 6 | Presentation | Hold `[tune]` **1000 ms** (`DISPLAY_BOOT_LIGHT_TEST_MS`) |
| 7 | TM1637 | Blank grids (TM1637 stays powered; not `0x80` display-off) |
