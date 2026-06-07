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


## Brightness

- Configurable via MQTT: 4 levels (1 = dimmest, 4 = brightest).
- The user can set a preferred brightness level that persists across
power cycles.

## Power management

- The display is powered off during sleep mode.
- On wake the display powers back on and resumes the selected mode.

