# Weight compensation

serves:
  - ../20-stories/feeding.md

Applies only in **compensated** dispense mode (`feed/mode = compensated`).
In open-loop mode, bursts run without weight feedback.

## Post-burst bowl weight check

After all planned bursts in a batch complete:

| Step | Action | Timing |
|------|--------|--------|
| 1 | De-assert motor EN (P0.1 low) | Immediate |
| 2 | Wait for mechanical settle (bowl vibration dampens) | `[tune]` 3 s |
| 3 | Power on CS1270 if not already on (P0.2 high) | — |
| 4 | Read bowl weight via UART2 (dispense-rate sampling ~500 ms) | — |
| 5 | Compute `grams_delivered = current_weight − weight_at_dispense_start` | — |

## Extra burst calculation

If `grams_delivered < target_grams`:

```
deficit = target_grams − grams_delivered
extra_bursts = max(1, deficit / 10)
```

Run `extra_bursts` using the same motor sequence as the initial batch
(see `dispense-cycle.md`), then re-measure.

## Give-up logic

Track consecutive batches where bowl weight delta is zero or negative
(no food reached the bowl despite motor running):

- Counter increments on each batch where `delta ≤ 0`.
- Counter resets on any batch with `delta > 0`.
- After `[tune]` 3 consecutive zero-change batches → give up.

On give-up:

1. Check hopper IR (see `hopper-sensing.md`): if `level = low`, set outcome = `empty_hopper`.
2. Otherwise set outcome = `underfill`.
3. Publish final result to MQTT `.../dispense/status`.

## Accuracy tolerance

- If `grams_delivered ≥ target_grams − [tune] 2 g`, treat as `success`
  (load cell resolution is ~1 g; don't chase rounding errors).
- If `grams_delivered ≥ target_grams`, stop immediately — do not over-dispense.

## Interaction with weighing subsystem

The dispense supervisor owns session state; the weigh driver does not (see
[weighing.md](weighing.md) **Weigh driver boundary**).

| Dispense supervisor | `weight_port` |
|---------------------|---------------|
| `weight_at_dispense_start` = `read_grams()` before motor | Stateless; no remembered zero |
| `grams_delivered` = current `read_grams()` − start | Each read is absolute food g |
| Extra bursts, give-up counters | Power rail + UART only |

- Compensation loop uses **dispense-rate** sampling (~500 ms) via `read_grams`.
- UART2 is serialized: idle weight sampling must not overlap with compensation reads.
- CS1270 must remain powered throughout the dispense + compensation cycle;
  power-off only after final outcome determined.

## Data recorded

| Field | Owner | Value |
|-------|-------|-------|
| `last_dispense_actual` | Dispense supervisor | Final `grams_delivered` after all batches |
| `eaten_today` | Monitoring | Updated from dispense history + bowl snapshots (not weigh driver) |
| Published to | — | `.../weight` and `.../dispense/status` |
