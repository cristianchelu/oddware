# CS1270 — Load-cell weighing ASSP

## Summary

Application-specific signal processor for resistive load cells. Provides
on-chip ADC, calibration storage, and a serial command interface. Connects
to the MT7682 via UART2 and measures the stainless bowl weight.

## Interface to host

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus | UART, half-duplex command/response | `[ds:CS1270]` |
| TX pin | MT7682 GPIO11 | `[probe]` |
| RX pin | MT7682 GPIO12 | `[probe]` |
| Baud rate | `[tune]` — likely 9600 or 115200 | |
| Power enable | AW9523B P0.2 (active high) | `[probe-needed]` |

## Key capabilities (from datasheet)

- Configurable weighing range (1–20 kg typical) `[ds:CS1270 §3]`
- 1 g resolution (application-dependent) `[ds:CS1270 §3]`
- On-chip zero-point and span calibration
- Temperature compensation
- Command-based interface: read weight, calibrate, set parameters

## Application notes

- Power-gated via GPIO expander to save current when not weighing.
- Must initialize and calibrate after power-on before readings are valid.
- Serialized access: only one task reads the weigh ASSP at a time (the
  dispense supervisor and battery/idle tasks must coordinate).
- Boot log shows format: `1270: zero point <N>`, weight as signed grams. `[bootlog]`
- Calibration procedure: tare (empty bowl) → place known weight → store span.
  User-facing calibration exposed via MQTT action.
