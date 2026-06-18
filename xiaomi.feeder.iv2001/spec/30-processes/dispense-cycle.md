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
- Event `target_g` = `N × 10` (design estimate) until gram requests land.

### Modes

| Mode | Behavior | Selection |
|------|----------|-----------|
| **Open-loop** | Run all planned bursts, measure bowl delta, publish completion event | Default (`feed/mode = open_loop`) |
| **Compensated** | After each batch, compare bowl delta to target; compute extra bursts if under | Opt-in via MQTT (`feed/mode = compensated`) |

Bowl **deltas** (`bowl_after − bowl_before`) are computed by the dispense
supervisor using two `read_grams` calls — not by runtime tare or offsets in the
weigh driver ([weighing.md](weighing.md) **Weigh driver boundary**).

## Pre-dispense baseline

Before the first motor EN assert:

1. If the last idle bowl sample is **< `[tune]` 2 s** old, use it as
   `weight_at_dispense_start`.
2. Otherwise perform a blocking `read_grams` before motor start.

Idle sampling runs every `[tune]` 500 ms on `EVT_DISPLAY_TICK` while no dispense
job is active ([app-event-loop.md](app-event-loop.md)).

## Post-dispense weigh (open-loop and compensated)

After motor completion (success or stuck), before the job ends:

| Step | Action | Timing |
|------|--------|--------|
| 1 | De-assert motor EN | Immediate (motor_ctrl) |
| 2 | Wait for mechanical settle | `[tune]` 3 s |
| 3 | Blocking `read_grams` | — |
| 4 | `grams_delivered = post − baseline`; clamp event `grams` ≥ 0 | — |
| 5 | Publish MQTT `.../dispense/event`; refresh `.../bowl_weight` | — |

While settling, `dispense_is_active()` remains true (dispensing pictograph
blinks). Job completion is **not** tied to `EVT_BURST_DONE` alone.

When scale reads fail or `bowl_error` is active, the event still fires with
`grams_estimated: true` and motor fallback `grams = portions × 10`.

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

## Dispense job scope

The dispense supervisor owns one **job-active** flag from request accept until
post-settle weigh completes and the completion event is published (or dropped
when MQTT offline). It is **not** tied to `motor_port.is_active()`:

- Open-loop portion dispense: job stays active from request accept through
  motor completion, `[tune]` 3 s settle, post-read, and event publish.
- Compensated gram dispense: job remains active through optional extra batches
  until target grams are met or the outcome is faulted — see
  [weight-compensation.md](weight-compensation.md).

While the job is active, `dispense_is_active()` is true, the dispensing
pictograph blinks, and idle bowl-gram sampling on `EVT_DISPLAY_TICK` is
suspended ([app-event-loop.md](app-event-loop.md)).

Job completion is **event-driven** (`EVT_BURST_DONE`, `EVT_MOTOR_FAULT`,
settle timer in `dispense_poll`, or future compensate / cancel paths). There is
no fallback that infers completion from `motor_port.is_active()` going false.

## Zero-delta counter (hopper foundation)

After motor runs, track consecutive jobs where **raw** bowl delta
(`post − baseline`, signed) is ≤ 0:

- Increment counter when raw delta ≤ 0 after motor ran.
- Reset counter when raw delta > 0.
- Future: after `[tune]` 3 consecutive zero-delta jobs with hopper IR low →
  outcome `empty_hopper` and hopper level transition (see `hopper-sensing.md`).

v1: counter is internal only (logged / test-visible); hopper MQTT topic unchanged.

## Dispense queue

- Only one dispense job executes at a time.
- Additional requests (schedule, MQTT, button) enter a FIFO queue.
- Maximum queue depth: `[tune]` 4 entries.
- Queued requests are serviced in order after current job completes.
- Queue overflow: reject with `aborted` status (future).

## Completion outcomes

Every terminal job publishes one MQTT dispense event (when online):

| Outcome | Condition |
|---------|-----------|
| `success` | Target met (compensated) or all bursts ran (open-loop) |
| `underfill` | Compensated mode measured delivery below target after retries |
| `stuck` | Anti-jam retries exhausted (see `jam-detection.md`) |
| `empty_hopper` | Motor ran, raw bowl delta ≤ 0, hopper IR confirms low (future) |
| `aborted` | Queue overflow, policy rejection, or user cancel via MQTT (future) |

v1 firmware emits `success` and `stuck` only.

Completion is reported via `.../dispense/event` only — no retained
`dispense/status` or in-progress `dispense/progress` topics.
