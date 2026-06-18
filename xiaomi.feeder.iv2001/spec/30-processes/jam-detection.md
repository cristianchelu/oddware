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

Applies to **dispense bursts only** (`MOTOR_PHASE_RUN_BURST`), not seal parking.

While motor EN (P0.1) is asserted during a burst:

- Count falling edges on P0.7 (motor-index IR detector).
- If motor runs longer than `[tune]` burst duration (8 s) with fewer index
  pulses than expected (at least 1 pulse expected per burst), treat as
  potential jam.
- Index-pulse timeout is a secondary signal; ADC-based detection is primary.

Seal parking (`MOTOR_PHASE_RUN_PARK`) does **not** use this rule — zero pulses
while searching for beam-open is normal. Park ends on beam-open, `[tune]` 4
index pulses, or the shared `[tune]` 20 s session cap (see
[dispense-cycle.md](dispense-cycle.md) § Index parking).

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

## UART bring-up logging

While `motor_ctrl` owns the motor, jam and recovery steps log on UART0 via
`app_log` (tag `motor`; bench aid; MQTT fault publish is separate). Full
lines use [app-logging.md](app-logging.md) format; table shows message body
only.

| Event | UART line (message body) |
|-------|--------------------------|
| Jam from GPIO17 ADC ISR | `jam: adc isr (> 1800 mA)` |
| Jam from polled ADC instant threshold | `jam: adc instant <mA> mA (> 1800 mA)` |
| Jam from polled ADC sustained threshold | `jam: adc sustained <mA> mA for <ms> ms (> 500 mA)` |
| Jam from burst index timeout | `jam: index timeout (0 pulses in <ms> ms, burst)` |
| Anti-jam reverse step | `antijam: retry <n>/3 reverse 1000 ms` |
| Anti-jam wiggle forward | `antijam: load <mA> mA still high, wiggle forward 500 ms` |
| Anti-jam wiggle reverse | `antijam: load <mA> mA still high, wiggle reverse 500 ms` |
| Anti-jam cleared | `antijam: load <mA> mA ok, resuming <burst\|park>` |
| Stuck after anti-jam exhausted | `stuck: antijam retries exhausted (last jam: <reason>)` |
| Stuck on session cap | `stuck: session timeout (20000 ms)` |
| Stuck on index I/O | `stuck: index I/O failed 3 times` |
| Stuck on driver stop failure | `stuck: motor stop failed` |
| Park already aligned | `park: already aligned (beam open)` |

`<reason>` is one of: `adc isr`, `adc instant`, `adc sustained`, `index
timeout`, `session timeout`.

## Stuck fault declaration

If anti-jam retries exhausted:

1. De-assert EN (P0.1 low) — motor off.
2. Disable motor-index IR LED (P0.6 low).
3. Set dispense outcome = `stuck`.
4. Publish completion event via MQTT `.../dispense/event` with `"event_type": "stuck"`.
5. Abort remaining dispense queue.
6. Recovery: user must clear physical jam and re-trigger via MQTT or button.

## Constraints

- Motor-load ADC sampling and index-pulse edge detection must be low-latency
  (sub-millisecond response) to catch stalls before mechanical damage. `[design]`
- Mux must not be switched to battery path (P1.7 high) while motor is running;
  battery reads happen only when motor is idle.
