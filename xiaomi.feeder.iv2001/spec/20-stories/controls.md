# Controls

## Overview

The feeder has three physical buttons and a child-lock feature.

## Manual dispense button (front)

| Gesture | Action |
|---------|--------|
| Short press | Dispense one default portion. |
| Long press | Reserved (design option: double portion or ignore). |

- Blocked when child lock is active (see Child lock).
- If a dispense is already running the request is queued.

## Power button (rear)

| Gesture | Action |
|---------|--------|
| Short press | Wake from sleep. |
| Long press (~3 s) | Enter sleep mode (motor off, display off, Wi-Fi off). |

- Blocked when child lock is active (see Child lock).
- Otherwise always responsive, including from sleep mode.

## Pin-hole reset button (recessed)

| Gesture | Action |
|---------|--------|
| Short press | Re-enter provisioning (AP mode) temporarily. |
| Long press (~7 s) | Factory reset — clears all config and reboots into first-boot provisioning. |

- Blocked when child lock is active (see Child lock).

## Child lock

- Activated by holding the reset and dispense buttons together (~3 s). The timer
  starts on the first press; the second button may follow within a few hundred
  milliseconds.
- Also controllable via MQTT.
- When active, **all physical button gestures are ignored** except the same
  reset+dispense combo, which toggles child lock off.
- MQTT commands are unaffected.
- A blocked physical gesture shows brief lock feedback on the display (blank
  digits, blinking lock pictograph).
- Steady lock pictograph while child lock remains active.
- Setting is persistent across power cycles.
