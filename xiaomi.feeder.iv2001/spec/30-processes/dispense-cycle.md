# Dispense cycle

serves:
  - ../20-stories/feeding.md

## Burst planning

### Gram targets (product)

A dispense request carries a **target_grams** value (clamped to 5–150 g).

- `bursts_planned = target_grams / 10` (integer division, minimum 1). `[design]`
- Each burst is sized to deliver approximately 10 g. `[tune]`

### Portion targets (bench UART)

A portion request carries **N** portions (`1 ≤ N ≤ 15`). `[design]`

- One continuous motor session with `pulse_target = N`.
- Motor does **not** stop between index holes while portions remain; holes
  count only until the Nth pulse de-asserts EN on the last hole (mechanical
  park).
- No separate `motor park` step for portion dispense.

### Modes

| Mode | Behavior | Selection |
|------|----------|-----------|
| **Open-loop** | Run all planned bursts, check bowl changed, done | Default (`feed/mode = open_loop`) |
| **Compensated** | After each batch, compare bowl delta to target; compute extra bursts if under | Opt-in via MQTT (`feed/mode = compensated`) |

Bowl **deltas** (`bowl_after − bowl_before`) are computed by the dispense
supervisor using two `read_grams` calls — not by runtime tare or offsets in the
weigh driver ([weighing.md](weighing.md) **Weigh driver boundary**).

## Motor sequencing per burst

Motor is controlled through AW9523B (I2C @ 0x58) driving SGM42507.

| Step | Action | Pin | Timing |
|------|--------|-----|--------|
| 1 | Assert PH = forward | P0.0 high | — |
| 2 | Wait direction-setup delay | — | `[tune]` 100 ms `[ds:SGM42507]` |
| 3 | Assert EN = run | P0.1 high | — |
| 4 | Motor runs until index pulse count reached (portion mode) or batch duration elapsed | P0.7 beam-open edges | `[tune]` index-timeout 8 s if zero pulses; `[tune]` N pulses for N-portion run |
| 5 | De-assert EN (coast) | P0.1 low | — |
| 6 | Gram batch only: if more bursts remain in batch, repeat from step 1 | — | Portion mode: single continuous run through step 5 |

Hard safety cutoff: motor must not run continuously beyond `[tune]` 20 s.
`motor_ctrl` enforces this on the **active burst/park session** (from first EN
assert through anti-jam), independent of the driver `running` flag. If AW9523B
I2C cannot de-assert EN, the session still ends with `stuck` and `motor_ctrl`
stops polling expander pins.

Production dispense uses `motor_ctrl` (not UART bench CLI). Bench
`motor fwd <ms>` and `motor rev <ms>` exercise the same PH/EN timed sequence
without index LED, ADC, or pulse counting — see
[uart-console.md](uart-console.md) § motor commands (operator must visually
confirm bench safety before each run).

## Index pulse tracking during burst

- Motor-index IR LED enabled on P0.6 at burst start, disabled at burst end.
- Detector on P0.7 (IRQ-capable): falling-edge = beam restored after hole passes.
- Index disk has 2 holes at 180° → 2 pulses per revolution. `[probe]`
- Burst can terminate on pulse count target instead of fixed duration (whichever first).

## Index parking (moisture seal)

After all bursts complete (no more queued), the auger must park at the IR-beam
alignment position for moisture seal:

1. Assert PH forward, EN run — motor runs **continuously** at low speed (single
   run, not burst pulses).
2. Stop when P0.7 detector reads beam-open (index hole aligned with beam path) —
   parking position per index slot.
3. If alignment not achieved within `[tune]` 4 index pulses, accept current
   position and de-assert EN.

Dispense bursts use the same forward direction and index feedback; each burst
runs EN uninterrupted until its duration or pulse target. Parking is the
final continuous run after the last burst, stopping only on the slot condition.

## Dispense queue

- Only one dispense job executes at a time.
- Additional requests (schedule, MQTT, button) enter a FIFO queue.
- Maximum queue depth: `[tune]` 4 entries.
- Queued requests are serviced in order after current job completes.
- Queue overflow: reject with `aborted` status.

## Dispense progress reporting

Publish 0–100 % to MQTT `.../dispense/progress` while active:

- Weight-based: `grams_delivered / target_grams × 100`
- Motor-based: `bursts_completed / bursts_planned × 100`
- Publish whichever is greater (capped at 100 %).
- Update interval: `[tune]` 1000 ms during active dispense.

## Completion outcomes

| Outcome | Condition |
|---------|-----------|
| `success` | Target met (compensated) or all bursts ran (open-loop) |
| `underfill` | Compensated mode measured delivery below target after retries |
| `stuck` | Anti-jam retries exhausted (see `jam-detection.md`) |
| `empty_hopper` | Motor ran, bowl delta ≈ 0, hopper IR confirms low (see `hopper-sensing.md`) |
| `aborted` | Queue overflow, policy rejection, or user cancel via MQTT |
