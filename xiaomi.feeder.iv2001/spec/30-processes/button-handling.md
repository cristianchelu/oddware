# Button handling

serves:
  - ../20-stories/controls.md

## Button inventory

| Button | Pin | Type | Provenance |
|--------|-----|------|------------|
| Rear power | AW9523B P0.3 | Tactile, active-low | `[probe]` |
| Pin-hole reset | AW9523B P0.4 | Recessed, active-low | `[probe]` |
| Manual dispense | AW9523B P1.0 | Tactile, active-high | `[probe]` |

All three are inputs on the AW9523B GPIO expander (I2C @ 0x58).

## IRQ dispatch

Tactile power (P0.3) and dispense (P1.0) share the AW9523B INT line with motor
index, mains sense, and hopper IR — all routed to MT7682 GPIO4 (EINT). The ISR
posts `EVT_BUTTON_IRQ` only (and optional `xTaskNotifyFromISR` for motor index
when armed); no I2C in the ISR (`wfci-bus-arbitration.md`). GPIO4 EINT setup:
`aw9523_irq_adapter.c`.
The `app` task samples `button_port` after debounce (see below). AW9523B input
register reads clear the expander interrupt. `[ds:AW9523B]`

IRQ mask registers 0x06/0x07 enable expander IRQ sources (`0` = enabled,
`1` = masked). Defaults in `board_gpio_iv2001.h`. Hopper broken-beam IR will
use the same GPIO4 wake path with its own adapter — not `gpio_expander_port`
from application code.

## Software debounce

All buttons debounced in software:

- Require `[tune]` 50 ms of stable state before registering a press or release.
- On `EVT_BUTTON_IRQ`, arm `[tune]` 50 ms before IRQ-backed buttons (power,
  dispense) accept new samples; pin-hole reset (no expander IRQ) is sampled on
  every `EVT_DISPLAY_TICK` without that gate.
- Two consecutive identical `button_port` reads satisfy the stable-state
  requirement (typically two `[tune]` 50 ms polls).
- After stable state changes, `button_input` emits a **transition**:
  **DOWN** (released → pressed) or **UP** (pressed → released). Gesture logic
  consumes these transitions plus `now_ms` from each poll.

## Bring-up UART logging

Until gesture actions (dispense, sleep, provisioning) are wired, a debounced
**DOWN** transition on any of the three buttons prints one UART line on the
console ([app-logging.md](app-logging.md); tag `app`; message body only below):

| Button | AW9523B pin | Message body |
|--------|-------------|--------------|
| Rear power | P0.3 | `btn power pressed` |
| Pin-hole reset | P0.4 | `btn reset pressed` |
| Manual dispense | P1.0 | `btn dispense pressed` |

- P0.3 and P0.4 are active-low; P1.0 (main-PCB SW2) is active-high — polarity
  is decoded in `button_port_adapter`; **pressed** is `true` on `button_port`
  (`read_sample`) for all three.
- P0.4 has no expander IRQ; it is sampled on each `[tune]` 50 ms
  `EVT_DISPLAY_TICK` (same cadence as presentation refresh).
- P0.3 and P1.0 also wake `EVT_BUTTON_IRQ` from GPIO4; `button_input` reads
  `button_port` after the IRQ debounce window — not in the ISR.
- Classified gestures log UART lines at `debug` (tag `app`; see
  [app-logging.md](app-logging.md)). Wired gestures also perform their
  actions below.

## Gesture state machine

`button_gesture` classifies debounced DOWN/UP transitions and hold duration.
Long press fires **once** when the hold threshold is crossed (finger may still
be down). Short press fires **only on UP** when hold duration is below the long
threshold and no long press fired for that down cycle.

| Button | Short (on UP) | Long (while held) | Long threshold |
|--------|---------------|-------------------|----------------|
| Dispense (P1.0) | release before long fired, hold &lt; `[tune]` 1 s | hold ≥ `[tune]` 2 s | `[tune]` 2000 ms |
| Power (P0.3) | release before long fired, hold &lt; `[tune]` 1 s | hold ≥ `[tune]` 3 s | `[tune]` 3000 ms |
| Reset (P0.4) | release before long fired, hold &lt; `[tune]` 1 s | hold ≥ `[tune]` 7 s | `[tune]` 7000 ms |
| Combo (P0.4 + P1.0) | — | both held ≥ `[tune]` 3 s | `[tune]` 3000 ms |

Combo timer starts from the **earlier** of the two button down times once both
are held (staggered press within a few hundred ms still counts as one 3 s hold).

Combo emits `CHILD_LOCK_TOGGLE` (not per-button). Child-lock policy (block
every physical gesture except combo when locked) is applied in `app`, not in
`button_gesture`.

While reset and dispense are both held and the combo has not fired yet,
per-button **long** gestures on those two buttons are suppressed so dispense
long (`[tune]` 2 s) does not preempt the `[tune]` 3 s combo. After the combo
fires, reset/dispense short and long are suppressed until both buttons are
released so lock-on/off does not look like a blocked gesture.

### Manual dispense (P1.0)

| Gesture | Action |
|---------|--------|
| Short | Dispense one portion (`feed/default_g`, default `[tune]` 10 g) |
| Long | `[design]` dispense double portion or ignore |

- Blocked when child lock is active: no action; blank digits and blink
  `DISPLAY_ICON_CHILD_LOCK` at `[tune]` 200 ms on / 200 ms off for
  `[tune]` 1 s per [display-presentation.md](display-presentation.md) § Lock
  indicator; log `child_lock blocked` at `info`.
- Short press calls `dispense_submit_portions(1)` when unlocked.
- Queued if dispense already in progress.
- Gesture UART at `debug`: `btn dispense short` / `btn dispense long`.

### Rear power (P0.3)

| Gesture | Action |
|---------|--------|
| Short | Wake from sleep / toggle Wi-Fi indicator |
| Long | Enter sleep mode (see `power-state-machine.md`) |

- Blocked when child lock is active (see § Child lock).
- IRQ remains enabled in sleep mode for wake-up.
- Bring-up stub UART: `btn power short` / `btn power long`.

### Pin-hole reset (P0.4)

| Gesture | Action |
|---------|--------|
| Short | Re-enter AP mode temporarily (30 s timeout, see `provisioning-flow.md`) |
| Long | Full factory reset: clear all NVDM, reboot into provisioning |

- Blocked when child lock is active (see § Child lock).

## Child lock

### Activation

- **Physical:** hold P0.4 (reset) + P1.0 (dispense) for `[tune]` 3 s → toggle
  child lock state. The 3 s window starts when the **first** of the two buttons
  goes down; the second may follow within a few hundred ms.
- Toggles `feed/child_lock` in NVDM; sets or clears `DISPLAY_ICON_CHILD_LOCK`
  (steady on when active); logs `child_lock on` / `child_lock off` at `info`.
  No blocked-feedback blink on toggle — only the steady icon changes. Gesture
  UART at `debug`: `btn child_lock toggle`.
- **MQTT:** `cmd/config {"child_lock": true|false}`.

### Behavior when locked

- **Only** the reset+dispense combo (`CHILD_LOCK_TOGGLE`) is handled; it
  toggles child lock off.
- Every other classified gesture on any button (dispense, reset, power — short
  or long) is ignored: no dispense, provisioning, sleep, or factory reset.
  Show lock feedback per `display-presentation.md` § Lock indicator; log
  `child_lock blocked` at `info`.
- MQTT commands: **unaffected** by child lock.

### Persistence

- Stored in NVDM key `feed/child_lock` (bool).
- Survives power cycles.
- Published as part of device config on `.../config`.
