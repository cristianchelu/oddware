# Feeding

## Overview

The feeder dispenses dry food accurately and safely, using a single auger
motor and a bowl-mounted weight sensor.

## Portion model

- A dispense request specifies a **target in grams** (range 5–150 g).
- Requests can come from a schedule, an MQTT command, or the manual
  dispense button.

## Dispense modes

The user can choose between two modes (persistent setting):

| Mode | Behavior |
|------|----------|
| **Open-loop** | Run the planned motor sequence and finish. No weight-based adjustment. |
| **Compensated** | After each batch, compare delivered weight to the target and add more if under. |

## Progress reporting

While a dispense is in progress the feeder publishes a 0–100 %
completion value so the user (or automation) can track it in real time.

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

- Only one dispense runs at a time; additional requests are queued
  (FIFO, small fixed depth).
- The feeder will not start an OTA update while a dispense is active.
