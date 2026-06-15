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

Handled inside `motor_index_port` (adapter owns P0.6 / P0.7 sequencing):

- **`sense`** — illuminate, sample, de-illuminate (park pre-check, `index read`
  CLI). Callers ask “is the beam open?” without knowing about the LED.
- **`session_begin` / `session_end`** — illuminate for the full motor burst/park
  session; **`poll`** samples while the session is active.
- LED off during idle, battery, and sleep modes to save current.
- P0.7 is not valid with LED off — a blocked reading does not prove
  misalignment.

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
- One portion: `[tune]` 1 pulse (180° — one hole passes).
- N portions in one job: `[tune]` N pulses in a **single** EN-high session;
  intermediate holes do not stop the motor — count only until the last hole
  stops the auger.
- Two portions without a separate park step: `[tune]` 2 pulses (full revolution).

### Jam timeout detection

- If motor EN is asserted and zero pulses received within `[tune]` 8 s,
  the auger is stalled.
- Feeds into jam-detection logic (see `jam-detection.md`).

### Seal parking

After dispense completes:

1. Run motor forward **continuously** at low speed (single EN assertion, not
   burst pulses).
2. Monitor P0.7 for beam-open state (index hole aligned with detector path).
3. Stop motor when open state detected — auger at seal index slot. If already
   open at park start, no motion is required (positive alignment ID).
4. If not achieved within `[tune]` 4 index pulses, accept current position.
