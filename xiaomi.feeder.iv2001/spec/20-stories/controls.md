# Controls

## Overview

The feeder has three physical buttons and a child-lock feature.

## Manual dispense button (front)

| Gesture | Action |
|---------|--------|
| Short press | Dispense one default portion. |
| Long press | Reserved (design option: double portion or ignore). |

- Blocked when child lock is active.
- If a dispense is already running the request is queued.

## Power button (rear)

| Gesture | Action |
|---------|--------|
| Short press | Wake from sleep. |
| Long press (~3 s) | Enter sleep mode (motor off, display off, Wi-Fi off). |

- Always responsive, even from sleep mode.

## Pin-hole reset button (recessed)

| Gesture | Action |
|---------|--------|
| Short press | Re-enter provisioning (AP mode) temporarily. |
| Long press (~7 s) | Factory reset — clears all config and reboots into first-boot provisioning. |

## Child lock

- Activated by holding the reset and dispense buttons together (~3 s).
- Also controllable via MQTT.
- When active:
  - Manual dispense button is ignored.
  - Pin-hole reset short-press is ignored.
  - Long-press factory reset still works (safety escape).
  - MQTT commands are unaffected.
- The display briefly shows a lock indicator when a locked button is
  pressed.
- Setting is persistent across power cycles.
