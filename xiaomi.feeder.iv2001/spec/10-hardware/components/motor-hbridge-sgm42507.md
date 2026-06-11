# SGM42507 — H-bridge motor driver

## Summary

Low-voltage H-bridge with phase/enable control interface. Drives the single
hopper auger motor. Board marking: **SG55B**.

## Interface to host (via AW9523B)

| Parameter | Value | Source |
|-----------|-------|--------|
| PH (phase/direction) | AW9523B P0.0 | `[probe]` |
| EN (enable) | AW9523B P0.1 | `[probe]` |
| FAULT | shared with EN pin (active-low) | `[ds:SGM42507 §6]` |
| VM (motor supply) | barrel/battery voltage (~5–6 V) | `[probe-needed]` |

## Control truth table

| PH | EN | Motor state | Source |
|----|----|-------------|--------|
| X | 0 | Coast (outputs Hi-Z) | `[ds:SGM42507 §4]` |
| 1 | 1 | Forward (OUT1=H, OUT2=L) | `[ds:SGM42507 §4]` |
| 0 | 1 | Reverse (OUT1=L, OUT2=H) | `[ds:SGM42507 §4]` |

## Key specs

| Parameter | Value | Source |
|-----------|-------|--------|
| Supply voltage (VM) | 1.8–5.5 V | `[ds:SGM42507 §5]` |
| Continuous output current | 1.2 A | `[ds:SGM42507 §5]` |
| Logic level (VIH) | 1.2 V min | `[ds:SGM42507 §5]` |
| FAULT threshold | overcurrent / thermal | `[ds:SGM42507 §6]` |

## Application notes

- **Sequencing:** set PH first, then assert EN after a direction-setup delay
  of `[tune]` 100 ms (see [dispense-cycle.md](../../30-processes/dispense-cycle.md)
  § Motor sequencing). This avoids glitch current on direction change.
  Datasheet does not mandate a minimum delay but recommends PH-stable-before-EN.
- **Coast/stop:** de-assert EN (low) to let the motor coast. Brake mode (both
  outputs low) is not available in PH/EN mode.
- **Stall protection:** the FAULT output goes low on overcurrent. Since
  FAULT shares the EN pin, a low reading on P0.1 while EN was commanded high
  indicates a fault condition. Alternatively monitor motor-load ADC.
- **Single motor:** there is only one auger motor. "Anti-jam stirring" is
  reverse rotation on the same motor.
