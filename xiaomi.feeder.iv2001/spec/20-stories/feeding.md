# Feeding

## Overview

The feeder dispenses dry food accurately and safely, using a single auger
motor and a bowl-mounted weight sensor.

## Portion model

- A dispense request specifies a **target in grams** (range 5–150 g).
- Requests can come from a schedule, an MQTT command, or the manual
  dispense button.
- On the bench UART, `dispense` or `dispense portions <N>` (N = 1–15) runs
  open-loop portion dispense (~10 g design target per portion) without gram
  targeting or weight compensation. `dispense grams <G>` (5–150 g) follows the
  persisted compensation mode. Product paths use gram targets (5–150 g).

## Dispense modes

The user can choose between two modes (persistent setting) from Home Assistant
or UART `feed mode`:

| Mode | Behavior |
|------|----------|
| **Open-loop** | Run the planned motor sequence, measure bowl delta, publish completion event. No weight-based adjustment. |
| **Compensated** | After each batch, compare delivered weight to the target and add more if under. |

Delivered grams come from `read_grams` before and after each batch (dispense
supervisor), not from a zero offset inside the weigh driver — see
[weighing.md](../30-processes/weighing.md) **Weigh driver boundary**.

## Completion reporting

When a dispense job finishes, the feeder publishes a **Dispense completed**
Home Assistant event (MQTT `.../dispense/event`) with measured grams when the
scale is valid, or an estimated amount with `grams_estimated: true` when not.
Automations can trigger on each completion (`event_type`: `success`, `stuck`,
etc.) and read `source`, `grams`, and other properties.

There is no retained dispense status topic or live progress percentage.

## Completion outcomes

Every dispense finishes with one of these results:

| Outcome | Meaning |
|---------|---------|
| `success` | Target weight met (or acceptably close in open-loop mode). |
| `underfill` | Dispense completed but measured delivery fell short of the target. |
| `stuck` | The auger jammed and automatic recovery failed. |
| `empty_hopper` | The motor ran but no food reached the bowl; hopper appears empty. |
| `aborted` | A policy rule blocked the request, or the user cancelled it. |

## Anti-jam behaviour

If the motor stalls during a dispense, the feeder automatically
attempts to clear the jam by reversing and retrying. If recovery fails
after multiple attempts the dispense is aborted and a `stuck` fault is
reported.

## Constraints

- Only one dispense runs at a time; additional requests are rejected as busy
  until the current job completes (FIFO queue is a future enhancement).
- The feeder will not start an OTA update while a dispense is active.
