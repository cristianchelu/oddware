# Auto-tare (drift compensation)

serves:
  - ../20-stories/monitoring.md
  - ../20-stories/feeding.md

## Purpose

Slow temperature drift shifts raw `read_grams` even when bowl contents are
unchanged. Auto-tare is an internal RAM drift compensator in the
monitoring/presentation layer — the weigh driver stays stateless (see
[weighing.md](weighing.md) **Weigh driver boundary**).

This is **not** a user-facing tare command. There is no `weigh tare`. Bench
operators inspect auto-tare state via `tare show` ([uart-console.md](uart-console.md)).

## RAM model

| Variable | Scope | Role |
|----------|-------|------|
| `stable_grams` | RAM | User-visible bowl food weight belief (panel + `bowl_weight` MQTT) |
| `drift_offset_g` | RAM | Added to raw `read_grams` so `presented = raw + drift_offset_g` tracks `stable_grams` during drift |
| `stable_valid` | RAM | False until first anchor |
| `pending_calibration` | RAM | True after bowl removal / `bowl_error` until natural anchor or first-dispense anchor |

**Bowl removal / `bowl_error`:** `stable_valid = false`, `pending_calibration = true`,
`drift_offset_g = 0`. Edge detection runs on each idle weight sample in `app`
(not from display or MQTT sync).

**Calibration complete** (`pending_calibration = false`) when:

- `INITIAL_STABLE_STREAK` quiet samples after boot or bowl re-insert
- Post-dispense read (see [dispense-cycle.md](dispense-cycle.md))
- First dispense pre-motor anchor while `pending_calibration` is set

While `pending_calibration`: drift nudging is **disabled**. Presentation shows
raw calibrated grams until anchored.

```text
presented_g = raw_g + drift_offset_g   (when stable_valid and not pending)
presented_g = raw_g                    (when pending_calibration or not stable_valid)
```

## Idle drift

On each idle weight sample when **quiet**, `!pending_calibration`, `stable_valid`,
and the runtime dispense-active flag is false (see
[task-model.md](../40-architecture/task-model.md) § Runtime snapshot):

- **Quiet:** per-sample `|Δraw| ≤ DRIFT_RATE_MAX_G`
- Set `drift_offset_g = stable_grams − raw_g` so `presented_g` stays at
  `stable_grams`; `stable_grams` is unchanged
- If `|Δraw| > DRIFT_RATE_MAX_G`, do not adjust drift (wait for a future
  activity session when eating monitoring lands)

## Initial anchor

After boot or when `bowl_error` clears, require `INITIAL_STABLE_STREAK`
consecutive quiet samples, then anchor at the current raw reading:

- `stable_grams = raw_g`
- `drift_offset_g = 0`
- `stable_valid = true`
- `pending_calibration = false`

## Re-anchor triggers

| Trigger | Owner |
|---------|--------|
| Post-dispense settle read | `dispense.c` → `auto_tare_anchor(post)` |
| Initial quiet streak | `auto_tare.c` idle sample |
| First dispense after re-insert | `dispense.c` when `pending_calibration` — see [dispense-cycle.md](dispense-cycle.md) § Pre-dispense baseline |

## Suppression

- **During dispense:** no drift nudging (read runtime dispense-active flag); post-dispense anchor
- **`pending_calibration`:** no drift nudging
- **`bowl_error` active:** invalidate stable state; set `pending_calibration`

## `[tune]` parameters

| Parameter | Start | Notes |
|-----------|-------|-------|
| `DRIFT_RATE_MAX_G` | 1 g per 500 ms sample | Above this → not drift |
| `INITIAL_STABLE_STREAK` | 4 | ~2 s quiet before first anchor |

## UART CLI

See [uart-console.md](uart-console.md) § `tare` commands.
