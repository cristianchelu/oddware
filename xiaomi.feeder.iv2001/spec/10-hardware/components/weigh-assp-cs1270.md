# CS1270 — Load-cell weighing ASSP

## Summary

Application-specific signal processor for resistive load cells. Provides
on-chip ADC, EEPROM calibration storage, and a serial command interface.
Connects to the MT7682 via UART2 and measures load on the stainless bowl.

## Provided bowl

| Parameter | Value | Source |
|-----------|-------|--------|
| Empty bowl mass | 350 g | `[product]` |

Firmware uses this mass as the span-calibration reference and subtracts it
from corrected readings so an empty installed bowl reports 0 g food.

Application logic treats the weigh driver as stateless (NVDM cal only):
`read_grams` → absolute food grams now; event deltas live in dispense and
monitoring tasks. See [weighing.md](../../30-processes/weighing.md)
**Weigh driver boundary**.

## Interface to host

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus | UART, command/response | `[ds:CS1270 §2.1]` |
| TX pin | MT7682 GPIO12 (UTXD2) | `[probe]` |
| RX pin | MT7682 GPIO11 (URXD2) | `[probe]` |
| Baud rate | 9600 8N1 `[tune]` | `[ds:CS1270 §2.1]` |
| Power enable | AW9523B P0.2 (active high) | `[probe-needed]` |

## UART protocol

Host command frame (6 bytes): `[ds:CS1270 §3.2.1]`

| Byte | Value | Meaning |
|------|-------|---------|
| 0 | `A1` | Preamble |
| 1 | `5A` | Header |
| 2–4 | CMD3 CMD2 CMD1 | Command payload |
| 5 | CHECKSUM | `~(0x5A + CMD3 + CMD2 + CMD1) + 1` |

Response frame (6 bytes): `[ds:CS1270 §3.2.2]`

| Byte | Value | Meaning |
|------|-------|---------|
| 0 | `B2` | Preamble |
| 1 | `A5` | Header |
| 2–4 | CMD3 CMD2 CMD1 | Response payload |
| 5 | CHECKSUM | `~(0xA5 + CMD3 + CMD2 + CMD1) + 1` |

### Host commands

| Command | CMD3 CMD2 CMD1 | Purpose |
|---------|----------------|---------|
| Query weight | `CA C2 EE` | Read current weight |
| Runtime zero | `F6 6F EE` | On-chip runtime zero — **not used** `[ds:CS1270 §3.4]` |
| Calibrate | `3A 4C XX` | On-chip EEPROM cal — **not used** |

Firmware version query (`96 87 EE`) is deferred to a later slice; bench
hardware returns a weight frame for that command today.

On-chip cal (`3A 4C XX`) and runtime zero are **permanently unused** `[design]`:
span mass is encoded as whole kilograms (1–20 kg) in the command byte, while
this product calibrates in grams with a 350 g bowl reference. Coefficients
would also be lost on every P0.2 power cycle. Host-side 2-point cal in NVDM
replaces both; see [weighing.md](../../30-processes/weighing.md).

### Response types

Weight (CMD3 = `00` or `01`): grams = `(CMD2 << 8) | CMD1`; `01` = negative.

Status (CMD3 = `0F`):

| CMD2 CMD1 | Meaning |
|-----------|---------|
| `FF 88` | Boot warming (~1 s after power-on) |
| `F6 6F` | Runtime zero in progress |
| `CA 3A` | Capturing calibration zero |
| `CA 2B` | Capturing calibration weight point 1 |
| `CA 1C` | Capturing calibration weight point 2 |
| `CA FF` | Calibration success |
| `CA 24` | Uncalibrated or calibration failed |

### Timing

| Parameter | Value | Source |
|-----------|-------|--------|
| Min interval between transactions | 100 ms | `[ds:CS1270 §2]` |
| Expander rail settle after P0.2 high | 50 ms `[tune]` | `[design]` AW9523B output latch |
| Boot settle after P0.2 high | 1100 ms `[tune]` | `[ds:CS1270 §3.1]` t1 ≈ 1.03 s; **no WFCI loan** |
| Weight update period | ~100 ms | `[ds:CS1270 §3.1]` t2 |

## Key capabilities (from datasheet)

- Configurable weighing range (1–20 kg typical) `[ds:CS1270 §3]`
- 1 g resolution (application-dependent) `[ds:CS1270 §3]`
- On-chip EEPROM calibration (2- or 3-point)
- Runtime zero baseline saved across power cycles `[ds:CS1270 §3.4]`
- Temperature compensation

## Application notes

- Power-gated via GPIO expander to save current when not weighing.
- Host firmware stores 2-point calibration in NVDM (`calib/*`); on-chip EEPROM
  cal and runtime zero are never issued.
- Serialized access: only one task reads the weigh ASSP at a time.
- Boot log shows format: `1270: zero point <N>`, weight as signed grams. `[bootlog]`
