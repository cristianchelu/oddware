# Button handling

serves:
  - ../20-stories/controls.md

## Button inventory

| Button | Pin | Type | Provenance |
|--------|-----|------|------------|
| Rear power | AW9523B P0.3 | Tactile, active-low | `[probe]` |
| Pin-hole reset | AW9523B P0.4 | Recessed, active-low | `[probe]` |
| Manual dispense | AW9523B P1.0 | Tactile, active-low | `[probe]` |

All three are inputs on the AW9523B GPIO expander (I2C @ 0x58).

## IRQ dispatch

All button IRQs arrive via a single AW9523B INT line → MT7682 GPIO4 (EINT).

On GPIO4 interrupt:

1. Read AW9523B input register P0 (addr 0x00) and P1 (addr 0x01) via I2C.
2. Compare with previous state to determine which pin(s) changed.
3. Dispatch to the appropriate button handler.
4. Interrupt is cleared by the register read. `[ds:AW9523B]`

IRQ-enabled pins: P0.3 (mask bit 3), P0.7 (motor index), P1.0 (mask bit 0),
P1.1 (mains), P1.4 (hopper IR). Configured via registers 0x06/0x07
(0 = enabled, 1 = masked).

## Software debounce

All buttons debounced in software:

- Require `[tune]` 50 ms of stable state before registering a press or release.
- On IRQ, start debounce timer; re-read pin state after timer expires.
- If state still matches, register the event. If not, discard.

## Gesture detection

### Manual dispense (P1.0)

| Gesture | Detection | Action |
|---------|-----------|--------|
| Short press (< `[tune]` 1 s) | Release before threshold | Dispense one portion (`feed/default_g`, default `[tune]` 10 g) |
| Long press (> `[tune]` 2 s) | Hold exceeds threshold | `[design]` dispense double portion or ignore |

- Blocked when child lock is active (no response; see lock indicator in
  `display-presentation.md`).
- Queued if dispense already in progress.

### Rear power (P0.3)

| Gesture | Detection | Action |
|---------|-----------|--------|
| Short press (< `[tune]` 1 s) | Release before threshold | Wake from sleep / toggle Wi-Fi indicator |
| Long press (> `[tune]` 3 s) | Hold exceeds threshold | Enter sleep mode (see `power-state-machine.md`) |

- IRQ remains enabled in sleep mode for wake-up.

### Pin-hole reset (P0.4)

| Gesture | Detection | Action |
|---------|-----------|--------|
| Short press (< `[tune]` 1 s) | Release before threshold | Re-enter AP mode temporarily (30 s timeout, see `provisioning-flow.md`) |
| Long press (> `[tune]` 7 s) | Hold exceeds threshold | Full factory reset: clear all NVDM, reboot into provisioning |

## Child lock

### Activation

- **Physical:** hold P0.4 (reset) + P1.0 (dispense) simultaneously for
  `[tune]` 3 s → toggle child lock state.
- **MQTT:** `cmd/config {"child_lock": true|false}`.

### Behavior when locked

- Manual dispense button (P1.0): ignored (lock indicator per
  `display-presentation.md`).
- Pin-hole reset short press (P0.4): ignored.
- Pin-hole reset long press (P0.4, 7 s): **still works** (factory reset must
  always be accessible).
- Rear power (P0.3): **still works** (sleep/wake must always be accessible).
- MQTT commands: **unaffected** by child lock.

### Persistence

- Stored in NVDM key `feed/child_lock` (bool).
- Survives power cycles.
- Published as part of device config on `.../config`.
