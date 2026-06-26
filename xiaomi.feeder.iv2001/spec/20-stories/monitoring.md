# Monitoring

## Overview

The feeder continuously reports its state so the user can see what the
device is doing and whether it needs attention.

## Bowl weight

- The feeder reports the current weight of food in the bowl.
- The provided stainless bowl weighs 350 g; the weigh driver subtracts this
  so an empty installed bowl reads 0 (`weight_port.read_dg`, internal
  tenth-grams).
- The panel shows **rounded whole grams**; MQTT `bowl_weight` reports
  **presented** mass at 0.1 g precision (raw `read_dg` plus internal RAM drift
  offset — see [auto-tare.md](../30-processes/auto-tare.md)).
- This is not a user tare command — there is no `weigh tare`.
- Over MQTT the user sees bowl mass on `petfeeder/<device_id>/bowl_weight`
  (plain string with one decimal, retained). Home Assistant discovers a **Bowl weight** sensor
  (`device_class`: weight, unit g). Updates are change-driven — not every
  local display refresh — see [mqtt-protocol.md](../30-processes/mqtt-protocol.md)
  § Bowl weight.

## Eaten-today tracking

- **Not** computed inside the weigh driver. The monitoring task derives
  consumption from dispense history and/or `read_dg` snapshots before and
  after feeding (see [weighing.md](../30-processes/weighing.md) **Weigh driver
  boundary**).
- Cumulative food consumed since midnight (local time); resets at midnight.

## Hopper fill level

- Three-state indicator over MQTT: **normal**, **low** (almost empty — IR beam
  clear), and **empty** (confirmed out-of-food via dispense weight check with
  IR low, or compensated underfill after retries).
- Based on a break-beam sensor inside the hopper cavity plus dispense bowl
  delta when the hopper is almost empty.
- Checked after every completed dispense. On **mains**, also polled about every
  60 s while idle so a manual refill updates the level without waiting for the
  next feed. On **battery**, sensing runs after dispense only.
- Retained MQTT topic `petfeeder/<device_id>/hopper` (`normal` | `low` | `empty`).
  Home Assistant discovers a **Hopper level** enum sensor — see
  [mqtt-protocol.md](../30-processes/mqtt-protocol.md).
- See [hopper-sensing.md](../30-processes/hopper-sensing.md) for IR debounce,
  empty latch, and mains-vs-battery sampling.

## Calibration

- The user can calibrate the weight sensor via MQTT:
  - **Zero:** capture with bowl removed.
  - **Span:** capture with the provided bowl installed (350 g reference).
- Calibration values are persisted and survive power cycles.
- The feeder may suggest recalibration if it detects drift.

## Power source

- Reports whether the feeder is running on **mains** or **battery**.
- Over MQTT the user sees mains presence on `petfeeder/<device_id>/mains`
  (`ON` when barrel connected, `OFF` on battery). Home Assistant discovers a
  **Mains connected** binary sensor (`device_class`: power).
- Changes are reported immediately when the debounced power source switches.

## Battery level

- Reports battery percentage (0–100 %) on `petfeeder/<device_id>/battery`
  (plain integer, retained). Payload `unknown` when pack ADC reads exactly 0 mV
  (no cells) — Home Assistant shows the sensor unavailable.
- Optional diagnostic raw pack voltage on `petfeeder/<device_id>/battery_voltage`
  (plain integer mV, retained). Home Assistant discovers **Battery pack voltage**
  (`device_class`: voltage, unit mV, disabled by default).
- Pack voltage is sampled on `[tune]` **60 s** while on battery and
  `[tune]` **300 s** while on mains — see
  [battery-monitoring.md](../30-processes/battery-monitoring.md).
- MQTT publishes when percentage changes by at least 1 point, on known ↔ unknown
  transition, on connect snapshot, and after a mains/battery transition resample.
- Publishes a low-battery warning when the level is critically low (future).
- At very low battery the feeder disables non-critical functions
  (display, Wi-Fi) and preserves power for scheduled feeding (future).
