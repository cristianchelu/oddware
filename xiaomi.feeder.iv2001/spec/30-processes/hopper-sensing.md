# Hopper sensing

serves:
  - ../20-stories/monitoring.md
  - ../20-stories/feeding.md

Broken-beam IR sensor across the hopper cavity detects food level.

## Hardware mapping

| Signal | Pin | Type | Source |
|--------|-----|------|--------|
| IR drive | MT7682 GPIO0 | PWM or toggled output | `[probe]` `[bootlog]` |
| IR sense | AW9523B P1.4 | Input, IRQ-capable | `[probe]` |

## Beam logic

| Detector state | Meaning | Output |
|----------------|---------|--------|
| Beam blocked (P1.4 = food interrupts path) | Hopper has food | `level = normal` |
| Beam clear (P1.4 = unobstructed) | Hopper low / empty | `level = low` |

Polarity note: "beam clear" = nothing blocks the IR path = detector sees LED
= `low` condition. `[probe]`

## Sensing sequence

1. Pulse IR LED on GPIO0 (brief on-period sufficient for detector response).
2. Read P1.4 via AW9523B input register (I2C @ 0x58, register 0x01, bit 4).
3. Record result (blocked or clear).

## Debounce

Require `[tune]` 6 consecutive "low" readings at `[tune]` 1 s intervals before
transitioning `hopper_level` from `normal` to `low`.

- Single blocked reading during the debounce window resets the counter.
- Transition from `low` back to `normal` requires `[tune]` 3 consecutive
  "blocked" readings (hysteresis prevents flapping).

## Post-dispense check

After every completed dispense cycle (regardless of outcome):

1. Run one hopper sense cycle immediately.
2. If beam clear, begin debounce countdown.
3. Used by weight-compensation give-up logic: if hopper is `low` and bowl
   delta is zero, outcome = `empty_hopper`.

## Output

- `hopper_level`: `normal` | `low`
- `low` is the **almost-empty** logical flag: beam clear (hopper cavity unobstructed).
  It does **not** mean a dispense failed — only that the IR path sees low fill.
- Published to MQTT `.../hopper`: `{"level": "normal"}` or `{"level": "low"}`
- Retained topic; updated on transition only.

`empty_hopper` (dispense outcome) is a separate path: the auger completed without
jam but bowl weight delta is zero after compensated give-up **and**
`hopper_level = low` (see [weight-compensation.md](weight-compensation.md)).
Hopper sensing alone does not set dispense outcomes.

## Process module (`hopper_input`)

Host-testable debounce over `hopper_ir_port` (same layering as
`button_input` over `button_port`).

| API | Behavior |
|-----|----------|
| `hopper_input_init(port)` | Bind port; level starts `normal` |
| `hopper_input_notify_dispense_complete()` | Arm immediate sense on next poll (post-dispense check) |
| `hopper_input_poll(now_ms)` | Run sense when post-dispense pending, debounce interval elapsed, or `[tune]` 60 s background timer |
| `hopper_input_get_level()` | Debounced `normal` \| `low` |
| `hopper_input_almost_empty()` | `true` when `get_level() == low` |
| `hopper_input_pop_transition(...)` | Level edge since last pop (for MQTT / logging) |

On `normal` → `low` or `low` → `normal` transition, log
`[hopper] level low` / `[hopper] level normal` on UART (MQTT publish deferred).

Wired from `app` on `EVT_DISPLAY_TICK` (`hopper_input_poll`) and on
`EVT_BURST_DONE` / `EVT_MOTOR_FAULT` when a dispense cycle ends
(`hopper_input_notify_dispense_complete`).

## Periodic background check

When no dispense is active, run hopper sense at `[tune]` 60 s interval
to catch manual refill or gradual depletion.
