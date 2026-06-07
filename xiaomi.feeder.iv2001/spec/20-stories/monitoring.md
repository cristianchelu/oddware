# Monitoring

## Overview

The feeder continuously reports its state so the user can see what the
device is doing and whether it needs attention.

## Bowl weight

- The feeder reports the current weight of food in the bowl (in grams).
- The weight reading is tare-referenced (empty bowl = 0 g).
- The user can trigger a tare reset via MQTT or a button combo.

## Eaten-today tracking

- The feeder tracks cumulative food consumed since midnight (local time):
  total dispensed minus what remains in the bowl.
- Resets at midnight.

## Hopper fill level

- Two-state indicator: **normal** or **low**.
- Based on a break-beam sensor inside the hopper cavity.
- Also checked after every successful dispense.

## Calibration

- The user can calibrate the weight sensor via MQTT:
  - **Tare:** set current load as zero.
  - **Span:** place a known weight on the bowl and confirm.
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
