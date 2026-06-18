# Battery monitoring

serves:
  - ../20-stories/monitoring.md

## ADC path

Battery voltage is routed to the SoC ADC through the analog mux:

| Signal | Pin | Source |
|--------|-----|--------|
| Mux select | AW9523B P1.7 | `[probe]` |
| ADC input | MT7682 GPIO17 (COM output of NC7SB3157) | `[probe]` |

To read battery: set P1.7 = high → B1 (battery voltage divider) → COM → GPIO17.

## Mux exclusivity

- During dispense, mux stays on motor path (P1.7 = low) for jam detection.
- Battery reads happen **only** when motor is idle (commanded EN = P0.1 low —
  output latch, not pad sense; FAULT shares EN `[probe]`).
- After switching mux, wait `[tune]` 1 ms settling time before sampling. `[ds:NC7SB3157]`

## Sampling procedure

1. Verify motor is idle (P0.1 = low).
2. Set P1.7 = high (select battery path) via AW9523B I2C write.
3. Wait settling time.
4. Read GPIO17 ADC `[tune]` 10 times.
5. Average readings; discard outliers (> 2σ from mean if sample count permits).
6. Restore P1.7 = low (motor path) after sampling.

## ADC conversion

Pin voltage at GPIO17:

```
pin_mV = raw_adc × 2500 / 4095
```

Source: `[ds:MT7682]`.

Battery voltage (after the B1 divider):

```
battery_mV = pin_mV × batt_scale_x1000 / 1000
```

Default `[design]` **11000** (multiplier 11.0, nominal ~11:1 divider).
Per-device trim in NVDM `power/batt_scale_x1000` — see [uart-console.md](uart-console.md)
`adc cal`. Motor-load reads report mA via 1 Ω shunt (`[probe]`); no scale factor.

## Chemistry and discharge curves

Pack voltage (mV) maps to percentage via `battery_pct_from_mv(pack_mv, chem)`.
Chemistry is selected by `battery_chemistry_t` enum; future chemistries add a
new enum value and knot table. Runtime selection via NVDM `power/batt_chemistry`
(uint8, absent → `0` / `BATTERY_CHEM_AA_ALK_4S`) is reserved — not loaded yet.

| Enum | Chemistry | Pack |
|------|-----------|------|
| `BATTERY_CHEM_AA_ALK_4S` (0) | AA alkaline primary cell | 4× series |

**Default curve `BATTERY_CHEM_AA_ALK_4S`** (nominal 6.0 V fresh → ~4.0 V depleted):

| Pack mV | % | Source |
|---------|---|--------|
| ≥ 6000 | 100 | `[design]` |
| 5600 | 75 | `[design]` |
| 5200 | 50 | `[design]` |
| 4800 | 25 | `[design]` |
| 4400 | 10 | `[design]` |
| ≤ 4000 | 0 | `[design]` |

Interpolate linearly between knots; clamp below/above range. Refine with bench
measurements. `[tune]`

## Sample interval

| Power source | Interval | Source |
|--------------|----------|--------|
| Battery | `[tune]` 60 s | Faster updates matter when on battery |
| Mains | `[tune]` 300 s | Battery voltage is less critical on mains |

Power source from debounced `power_source_input` ([power-state-machine.md](power-state-machine.md)
§ Mains sense input), not a raw P1.1 read.

## Thresholds and actions

| Condition | Action | Source |
|-----------|--------|--------|
| `battery_pct < [tune] 10 %` | Publish low-battery warning via MQTT `.../battery` | `[design]` |
| `battery_pct < [tune] 5 %` | Disable non-critical functions (display off, Wi-Fi off); keep scheduled dispense + motor safety | `[design]` |

## Output

| Topic | Payload | When |
|-------|---------|------|
| `.../battery` | Plain integer `0`–`100`, or `unknown` when pack ADC is 0 mV | After ADC sample when % changes ≥ 1 pt, known ↔ unknown transition, connect snapshot, or forced resample |
| `.../battery_voltage` | Plain integer pack mV (including `0`) | Same sample tick as `.../battery` |
| `.../mains` | `ON` / `OFF` | Immediately on debounced mains connect/loss; connect snapshot |

`unknown` on `.../battery` means no pack / 0 mV — not depleted 0 %. Do not use
an empty MQTT payload: brokers drop zero-length retains. See
[mqtt-protocol.md](mqtt-protocol.md) § Battery.
