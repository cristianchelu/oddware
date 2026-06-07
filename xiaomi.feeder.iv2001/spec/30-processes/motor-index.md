# Motor index sensing

serves:
  - ../20-stories/feeding.md

IR broken-beam sensor on the auger shaft index disk provides rotational
feedback during dispense.

## Hardware mapping

| Signal | Pin | Direction | Source |
|--------|-----|-----------|--------|
| Index IR LED | AW9523B P0.6 | Output | `[probe]` |
| Index IR detector | AW9523B P0.7 | Input + IRQ | `[probe]` |

AW9523B at I2C address 0x58. INT line → MT7682 GPIO4.

## Index disk geometry

- 2 holes at 180° on the auger shaft disk. `[probe]`
- Each hole passing the detector produces one falling edge (beam restored).
- 2 pulses per full revolution.

## LED power management

- LED (P0.6) enabled **only** while motor runs for dispense.
- Set P0.6 high immediately before asserting motor EN (P0.1).
- Set P0.6 low immediately after de-asserting motor EN.
- Keeps LED off during idle, battery, and sleep modes to save current.

## Edge detection

- Detect on **falling edge** of P0.7 (beam restored after hole passes).
- P0.7 is IRQ-enabled via AW9523B interrupt register (register 0x06, bit 7 = 0).
- IRQ fires on change → read P0.7 level → if low-to-high transition = pulse.
- Alternatively: poll P0.7 at high rate during motor run if IRQ latency
  through I2C is insufficient. `[design]`

## Consumers

### Pulse-count progress

- Count pulses during each burst as secondary progress indicator.
- Burst can terminate when target pulse count reached (alternative to
  fixed-duration timeout).
- Typical: `[tune]` 2 pulses per burst (≈ 1 revolution).

### Jam timeout detection

- If motor EN is asserted and zero pulses received within `[tune]` 2 s,
  the auger is stalled.
- Feeds into jam-detection logic (see `jam-detection.md`).

### Seal parking

After dispense completes:

1. Run motor forward **continuously** at low speed (single EN assertion, not
   burst pulses).
2. Monitor P0.7 for beam-open state (index hole aligned with detector path).
3. Stop motor when open state detected — auger at seal index slot.
4. If not achieved within `[tune]` 4 index pulses, accept current position.
