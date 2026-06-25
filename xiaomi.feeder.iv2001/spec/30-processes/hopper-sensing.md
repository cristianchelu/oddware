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

| Detector state | Meaning | IR level |
|----------------|---------|----------|
| Beam blocked (P1.4 = food interrupts path) | Hopper has food | `normal` |
| Beam clear (P1.4 = unobstructed) | Hopper almost empty | `low` |

Polarity note: "beam clear" = nothing blocks the IR path = detector sees LED
= `low` condition. `[probe]`

## Sensing sequence

1. Pulse IR LED on GPIO0 (brief on-period sufficient for detector response).
2. Read P1.4 via AW9523B input register (I2C @ 0x58, register 0x01, bit 4).
3. Record result (blocked or clear).

Each `sense()` call performs one pulse + one read. Debounce streaks spread
multiple reads across wall time (one sample per eligible poll tick) — no
multi-read loop inside a single poll.

## Debounce

Require `[tune]` 6 consecutive "low" readings at `[tune]` 1 s intervals before
transitioning IR level from `normal` to `low`.

- Single blocked reading during the debounce window resets the counter.
- Transition from `low` back to `normal` requires `[tune]` 3 consecutive
  "blocked" readings (hysteresis prevents flapping).

## When IR is sampled

| Power source | Sampling |
|--------------|----------|
| **Mains** | Post-dispense (immediate) **and** idle background every `[tune]` **60 s** |
| **Battery** | Post-dispense only — no periodic background |

Active debounce intervals (1 s between streak readings while a transition is
pending) and post-dispense forced samples run on **both** power sources.

`app` passes `background_enabled = true` only when debounced power source is
mains (`power_source_input_get() == mains`).

## Post-dispense check

After every completed dispense cycle (regardless of outcome):

1. Run one hopper sense cycle immediately.
2. If beam clear, begin debounce countdown.

Dispense **event** outcomes and published **hopper** level are separate.
`underfill` on `.../dispense/event` does not require IR `low` at completion;
`.../hopper` = `empty` latches when the compositor decides the material path
is exhausted (see below).

## Published hopper level (MQTT)

Retained topic `.../hopper`: `normal` | `low` | `empty` (plain text).
Updated on transition only.

| Level | Meaning |
|-------|---------|
| `normal` | IR beam blocked (food in path); hopper not latched empty |
| `low` | IR beam clear (almost empty); hopper not latched empty |
| `empty` | Material path exhausted: measured zero delivery with IR `low`, or `underfill` after compensated retry exhaustion |

`low` is the **almost-empty** IR flag. It does **not** alone mean a dispense
failed. `empty` is stronger: out-of-food for feeding purposes. A compensated
`underfill` may latch `empty` even when the last few grams came from chute
stragglers — the hopper is effectively empty after that job.

## Process module (`hopper_input`)

Host-testable debounce over `hopper_ir_port` (same layering as
`button_input` over `button_port`). IR-only — outputs `normal` | `low`.

| API | Behavior |
|-----|----------|
| `hopper_input_init(port)` | Bind port; level starts `normal` |
| `hopper_input_notify_dispense_complete()` | Arm immediate sense on next poll (post-dispense check) |
| `hopper_input_poll(now_ms, background_enabled)` | Run sense when post-dispense pending, debounce interval elapsed, or (mains only) `[tune]` 60 s background timer |
| `hopper_input_get_level()` | Debounced `normal` \| `low` |
| `hopper_input_almost_empty()` | `true` when `get_level() == low` |
| `hopper_input_pop_transition(...)` | IR level edge since last pop |

On `normal` → `low` or `low` → `normal` transition, log on UART via
`app_log` (tag `hopper`; message body `level low` / `level normal`; see
[app-logging.md](app-logging.md)).

Wired from `app` on `EVT_DISPLAY_TICK` (`hopper_input_poll` with mains
background flag). Post-dispense notify is forwarded from `hopper_level` after
dispense job finish.

## Process module (`hopper_level`)

Compositor over `hopper_input` + dispense results. Owns the three-state
published model and MQTT transition queue.

| API | Behavior |
|-----|----------|
| `hopper_level_init()` | Reset; published level `normal` |
| `hopper_level_poll()` | Drain `hopper_input_pop_transition`, recompute composite, enqueue published edges |
| `hopper_level_on_dispense_finished(outcome, raw_delta, measured)` | Latch/clear empty; may upgrade outcome to `empty_hopper` |
| `hopper_level_notify_dispense_complete()` | Forward to `hopper_input_notify_dispense_complete()` |
| `hopper_level_get()` | Current published `normal` \| `low` \| `empty` |
| `hopper_level_pop_transition(...)` | Published level edge since last pop (for MQTT) |

**Published level:**

- If empty latched → `empty`
- Else if `hopper_input_get_level() == low` → `low`
- Else → `normal`

**Latch empty when:** open-loop `success` with measured `raw_delta ≤ 0` and IR
`low`; or compensated `underfill` after batch cap / give-up.

**Clear empty latch when:** IR recovers to `normal` (3 blocked streak), or
successful dispense with measured `raw_delta > 0`.

**Do not latch on `stuck`** — jam ≠ empty hopper.

While latched `empty`, IR `low` edges do not emit duplicate MQTT transitions.
Refill (`empty` → `normal`) skips `low`.

Wired from `app` on `EVT_DISPLAY_TICK` after `hopper_input_poll`. MQTT sync
drains `hopper_level_pop_transition` → `mqtt_hopper_sync`.
