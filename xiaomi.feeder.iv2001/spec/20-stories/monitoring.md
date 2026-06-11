# Monitoring

## Overview

The feeder continuously reports its state so the user can see what the
device is doing and whether it needs attention.

## Bowl weight

- The feeder reports the current weight of food in the bowl (in grams).
- The provided stainless bowl weighs 350 g; the weigh driver subtracts this
  so an empty installed bowl reads 0 g (`weight_port.read_grams`).
- This is an **absolute** reading — not a relative zero from a prior tare.

## Eaten-today tracking

- **Not** computed inside the weigh driver. The monitoring task derives
  consumption from dispense history and/or `read_grams` snapshots before and
  after feeding (see [weighing.md](../30-processes/weighing.md) **Weigh driver
  boundary**).
- Cumulative food consumed since midnight (local time); resets at midnight.

## Hopper fill level

- Two-state indicator: **normal** or **low** (almost empty — IR beam clear).
- Based on a break-beam sensor inside the hopper cavity.
- Checked after every completed dispense and periodically while idle.
- Distinct from the `empty_hopper` dispense outcome (motor ran but bowl weight
  did not increase); that path also requires `low` hopper level — see
  [hopper-sensing.md](../30-processes/hopper-sensing.md).

## Calibration

- The user can calibrate the weight sensor via MQTT:
  - **Zero:** capture with bowl removed.
  - **Span:** capture with the provided bowl installed (350 g reference).
- Calibration values are persisted and survive power cycles.
- The feeder may suggest recalibration if it detects drift.

## Power source

- Reports whether the feeder is running on **mains** or **battery**.
- Changes are reported immediately when the power source switches.

## Battery level

- Reports battery percentage (0–100 %).
- Publishes a low-battery warning when the level is critically low.
- At very low battery the feeder disables non-critical functions
  (display, Wi-Fi) and preserves power for scheduled feeding.
