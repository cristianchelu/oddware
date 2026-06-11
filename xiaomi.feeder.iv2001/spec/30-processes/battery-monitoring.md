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

## Discharge curve mapping

Map measured voltage to percentage using AA alkaline discharge curve
(4× AA in series, nominal 6.0 V fresh → ~4.0 V depleted):

| Voltage (approx) | Percentage | Source |
|-------------------|------------|--------|
| ≥ 6.0 V | 100 % | `[design]` |
| 5.6 V | 75 % | `[design]` |
| 5.2 V | 50 % | `[design]` |
| 4.8 V | 25 % | `[design]` |
| 4.4 V | 10 % | `[design]` |
| ≤ 4.0 V | 0 % | `[design]` |

Interpolate linearly between points. Curve should be refined with bench
measurements. `[tune]`

## Sample interval

| Power source | Interval | Source |
|--------------|----------|--------|
| Battery | `[tune]` 60 s | Faster updates matter when on battery |
| Mains | `[tune]` 300 s | Battery voltage is less critical on mains |

Power source determined by P1.1 level (mains-present sense).

## Thresholds and actions

| Condition | Action | Source |
|-----------|--------|--------|
| `battery_pct < [tune] 10 %` | Publish low-battery warning via MQTT `.../power` | `[design]` |
| `battery_pct < [tune] 5 %` | Disable non-critical functions (display off, Wi-Fi off); keep scheduled dispense + motor safety | `[design]` |

## Output

Published to MQTT `.../power`:
```json
{"source": "mains|battery", "battery_pct": <0-100>}
```
