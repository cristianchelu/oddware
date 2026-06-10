# Display

## Overview

The feeder has a three-digit 7-segment display with fixed pictographs that
shows contextual information at a glance.

## Display modes

The display shows one mode at a time, selected via MQTT. Idle alternatives
(**weight**, **eaten today**) are persisted across power cycles. The panel
has three digit positions and fixed pictographs only — no alphabetic or
time readout is possible.


| Mode            | Content                                              | When shown                      |
| --------------- | ---------------------------------------------------- | ------------------------------- |
| **Weight**      | Current bowl weight in grams (e.g. `42g`).           | Default idle mode.              |
| **Eaten today** | Cumulative grams consumed since midnight (e.g. `85g`). | User-selected idle alternative. |
| **Off**         | Blank.                                               | Sleep or user preference.       |


## Status pictographs

The panel has fixed pictographs beside the digits. They show device status
independently of the idle digit mode (weight / eaten today) — digits may be
blank while a pictograph is active.

### Wi-Fi indicator

- **Steady on** when connected to the home Wi-Fi network.
- **Blinking** (slower cadence) while connecting to Wi-Fi.
- **Faster blink** while in setup / AP provisioning mode.
- **Off** when not connected (failed connect, no link).

### MQTT / broker indicator (status lightbar)

The orange and green segments below the digits show MQTT broker status.
Wi-Fi and MQTT indicators are independent — both may be active at once.

- **Green steady on** when connected to the MQTT broker.
- **Orange mostly on** with a brief off-blip while connecting to the broker
  (inverted blink — lit most of the time). Bowl-gram digits in **weight** mode
  continue to update during this phase.
- **Orange signature blink** when the broker is unreachable or the session
  failed (two short off-blips, then a longer on, repeating).
- **Both off** when MQTT is not in use (no broker configured, provisioning,
  or Wi-Fi not ready).

## Brightness

- Configurable via MQTT: 4 levels (1 = dimmest, 4 = brightest).
- The user can set a preferred brightness level that persists across
power cycles.

## Power management

- The display is powered off during sleep mode.
- On wake the display powers back on and resumes the selected mode.

