# Jam detection

serves:
  - ../20-stories/feeding.md

Two parallel detection paths run continuously while the motor drives forward.

## Motor-load ADC monitoring

The analog mux NC7SB3157 routes motor load sense to MT7682 GPIO17 ADC when
select P1.7 = low. During dispense, the mux stays on motor path.

| Parameter | Value | Source |
|-----------|-------|--------|
| ADC pin | GPIO17 (COM output of NC7SB3157) | `[probe]` |
| Mux select | P1.7 = low (motor path B0→COM) | `[probe]` |
| ADC conversion | raw × 2500 / 4095 → mA | `[ds:MT7682]` + `[probe]` |
| Shunt | 1 Ω on motor path — pin mV numerically equals mA (no scale factor) | `[probe]` |

### Immediate jam threshold

- If motor-load reading > `[tune]` 1800 mA → instant jam signal.
- Single sample above threshold is sufficient; no averaging.

### Sustained load threshold

- If motor-load reading > `[tune]` 500 mA for longer than `[tune]` 4 s → jam signal.
- Sampled at sub-millisecond intervals during motor run.
- Uses a rolling timer: reset whenever ADC drops below threshold.

## Index-pulse timeout

While motor EN (P0.1) is asserted:

- Count falling edges on P0.7 (motor-index IR detector).
- If motor runs longer than `[tune]` burst duration (2 s) with fewer index
  pulses than expected (at least 1 pulse expected per burst), treat as
  potential jam.
- Index-pulse timeout is a secondary signal; ADC-based detection is primary.

## Anti-jam recovery sequence

On jam signal from either path:

| Step | Action | Timing |
|------|--------|--------|
| 1 | De-assert EN (P0.1 low) — stop forward | Immediate |
| 2 | Assert PH = reverse (P0.0 low), wait direction-setup delay | `[tune]` 100 ms |
| 3 | Assert EN — run reverse | `[tune]` 1 s reverse burst |
| 4 | De-assert EN, re-check ADC | — |
| 5 | If jam persists: toggle forward/reverse (wiggle sequence) | `[tune]` 500 ms each |
| 6 | Repeat up to `[tune]` 3 retries total | — |

## Stuck fault declaration

If anti-jam retries exhausted:

1. De-assert EN (P0.1 low) — motor off.
2. Disable motor-index IR LED (P0.6 low).
3. Set dispense outcome = `stuck`.
4. Publish fault via MQTT `.../dispense/status`: `{"state": "fault", "last_result": "stuck"}`.
5. Abort remaining dispense queue.
6. Recovery: user must clear physical jam and re-trigger via MQTT or button.

## Constraints

- Motor-load ADC sampling and index-pulse edge detection must be low-latency
  (sub-millisecond response) to catch stalls before mechanical damage. `[design]`
- Mux must not be switched to battery path (P1.7 high) while motor is running;
  battery reads happen only when motor is idle.
