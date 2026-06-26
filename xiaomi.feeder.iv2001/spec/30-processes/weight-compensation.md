# Weight compensation

serves:
  - ../20-stories/feeding.md

Applies only in **compensated** dispense mode (`feed/mode = compensated`).
In open-loop mode, bursts run without weight feedback but still measure
pre/post bowl delta for the completion event (see [dispense-cycle.md](dispense-cycle.md)).

## Post-burst bowl weight check

After all planned bursts in a batch complete:

| Step | Action | Timing |
|------|--------|--------|
| 1 | De-assert motor EN (P0.1 low) | Immediate |
| 2 | Wait for mechanical settle (bowl vibration dampens) | `[tune]` 3 s |
| 3 | Power on CS1270 if not already on (P0.2 high) | — |
| 4 | Read bowl weight via UART2 (dispense-rate sampling ~500 ms) | — |
| 5 | Compute `dg_delivered = current_dg − weight_at_dispense_start_dg` | — |

## Extra burst calculation

If `dg_delivered < target_dg` (`target_dg = target_grams × 10`):

```
deficit_dg = target_dg − dg_delivered
extra_bursts = max(1, deficit_dg / 100)
```

(`100` dg = 10 g per burst.)

Run `extra_bursts` using the same motor sequence as the initial batch
(see `dispense-cycle.md`), then re-measure.

## Give-up logic

Track consecutive batches where bowl weight delta is zero or negative
(no food reached the bowl despite motor running):

- Counter increments on each batch where `delta ≤ 0`.
- Counter resets on any batch with `delta > 0`.
- After `[tune]` 3 consecutive zero-change batches → give up.

On give-up or batch cap (same outcome rules as [dispense-cycle.md](dispense-cycle.md)):

1. Set dispense event outcome = `underfill`.
2. Publish completion event to MQTT `.../dispense/event`.
3. Latch published hopper MQTT level `empty` when the material path is
   exhausted (see [hopper-sensing.md](hopper-sensing.md)).

`underfill` means the bowl received less than the gram target after retries.
It is independent of hopper IR at the instant of give-up — the last grams may
be chute/auger stragglers. The hopper is often truly empty only after that
final underfilled delivery.

## Batch cap

Maximum `[tune]` **3** total motor batches per job (initial + compensation
rounds). When the cap is reached without meeting target → outcome `underfill`
and published hopper level `empty`.

## Accuracy tolerance

- If `dg_delivered ≥ target_dg − [tune] 50 dg` (5 g), treat as `success`.
- If `dg_delivered ≥ target_dg`, stop immediately — do not over-dispense.

## Interaction with weighing subsystem

The dispense supervisor owns session state; the weigh driver does not (see
[weighing.md](weighing.md) **Weigh driver boundary**).

| Dispense supervisor | `weight_port` |
|---------------------|---------------|
| `weight_at_dispense_start_dg` = fresh idle sample or `read_dg()` before motor | Stateless; no remembered zero |
| `dg_delivered` = current `read_dg()` − start | Each read is absolute food dg |
| Extra bursts, give-up counters | Power rail + UART only |

- Compensation loop uses **dispense-rate** sampling (~500 ms) via `read_dg`.
- UART2 is serialized: idle weight sampling must not overlap with compensation reads.
- CS1270 must remain powered throughout the dispense + compensation cycle;
  power-off only after final outcome determined.

## Data recorded

| Field | Owner | Value |
|-------|-------|-------|
| `last_dispense_actual` | Dispense supervisor | Final `dg_delivered`, rounded to whole g in event |
| `eaten_today` | Monitoring | Updated from dispense history + bowl snapshots (not weigh driver) |
| Published to | — | `.../dispense/event` (whole-gram `grams`) and `.../bowl_weight` (0.1 g presented — see [auto-tare.md](auto-tare.md)) |
