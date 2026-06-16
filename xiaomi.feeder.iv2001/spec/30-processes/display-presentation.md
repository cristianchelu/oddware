# Display presentation

serves:
  - ../20-stories/display.md
  - ../20-stories/controls.md

## Panel constraints

Three digit grids plus fixed pictographs on grids 3–4. Only numeric gram
readouts are supported — clock, alphabetic error codes, and
dispense-progress strings are not possible on this panel.

## Software layering

`[design]` This file owns **what** appears on the panel and **when**: modes,
power policy, lock spinner, connectivity indicators.

| Firmware module | Role |
|-----------------|------|
| `display_boot.c` | Boot light-test policy (0xFF fill, hold, blank) |
| `display_glyph.c` | Digit LUT and icon-label → grid-byte composition (pure, no I/O) |
| `display_presentation.c` | Scene state, blink, animation, periodic refresh |
| `display_wifi_indicator.c` | Wi-Fi pictograph policy (connecting, AP, connected, off) |
| `display_mqtt_indicator.c` | MQTT status lightbar policy (connecting, connected, error, off) |
| `display_dispense_indicator.c` | Dispensing pictograph policy (active blink, idle off) |
| `display_anim_builtin.c` | Const frame tables for OTA and lock-busy animations |
| MQTT `cmd/display` handler | Future mode/brightness commands |

**Dependency rule:** presentation modules call `display_port` only. They must
not include TM1637 command bytes, grid indices, AW9523B registers, or SDK
I2C/GPIO headers. How the panel is driven:
[display-driver.md](display-driver.md).

**Boot self-test:** On power-on, show all segments lit (`0xFF` on grids 0–4)
for `[tune]` 1000 ms, then black. Driver provides `show_fill` / `blank`; policy
lives here and in `display_boot.c`. Hardware steps:
[display-driver.md](display-driver.md) § Boot self-test.

## Wi-Fi indicator

`display_wifi_indicator.c` drives `DISPLAY_ICON_WIFI`. Wi-Fi lifecycle
producers (`wifi_sta.c`, `provision.c`, `provision_wifi_try.c`) post
`EVT_WIFI_STA_*` to `app_event_q`; the `app` task calls the indicator helpers.
See [app-event-loop.md](app-event-loop.md). User-facing semantics:
[display.md](../20-stories/display.md) § Wi-Fi indicator.

| State | Presentation | `[tune]` blink on/off |
|-------|--------------|----------------------|
| STA connecting | `display_wifi_indicator_connecting()` | 500 ms / 500 ms |
| AP provisioning active | `display_wifi_indicator_ap_mode()` | 150 ms / 150 ms |
| STA connected (DHCP OK) | `display_wifi_indicator_connected()` | steady on |
| Not connected / connect failed | `display_wifi_indicator_off()` | steady off |

Transitions:

| Event | From → To |
|-------|-----------|
| `wifi_sta_request_connect()` | off or prior → connecting blink |
| DHCP OK + valid IP | connecting → steady on |
| `connect()` or IP failure | connecting → steady off |
| AP + HTTP up (`provision_task`) | off → AP blink |
| Portal STA test (`provision_wifi_try_connect`) | AP blink → connecting blink |
| Portal STA test failed (AP restored) | connecting → AP blink |
| Successful portal save + reboot | (reboot) → connecting blink → steady on |

Portal STA test display steps: [provisioning-flow.md](provisioning-flow.md) §
Provisioning STA test-connect.

## Dispensing indicator

`display_dispense_indicator.c` drives `DISPLAY_ICON_DISPENSING`. The dispense
supervisor calls `display_dispense_indicator_active()` when a job is accepted
and `display_dispense_indicator_idle()` when the job completes or faults.
User-facing semantics: [display.md](../20-stories/display.md) § Dispensing
indicator.

| State | Presentation | `[tune]` blink on/off |
|-------|--------------|----------------------|
| Dispense job active | `display_dispense_indicator_active()` | 500 ms / 500 ms |
| Idle (no job) | `display_dispense_indicator_idle()` | steady off |

In future **compensated** gram dispense, the job (and blink) remain active
through post-motor weigh settle until bowl grams are stabilized and the display
is updated — see [weight-compensation.md](weight-compensation.md).

## MQTT indicator (status lightbar)

`display_mqtt_indicator.c` drives `DISPLAY_ICON_BAR_ORANGE` and
`DISPLAY_ICON_BAR_GREEN`. `mqtt_client_request_connect()` and
`mqtt_client_step()` post `EVT_MQTT_SESSION`; the `app` task calls the
indicator helpers. Helpers update presentation **scene state only** — they do
not call `display_presentation_refresh()`. Physical TM1637 updates come from
`EVT_DISPLAY_TICK` via `display_presentation_tick` → `try_show_grids`. See
[app-event-loop.md](app-event-loop.md) § Coexistence with MQTT connect.
User-facing semantics: [display.md](../20-stories/display.md) § MQTT / broker
indicator.

| State | Presentation | Effect |
|-------|--------------|--------|
| MQTT connecting | `display_mqtt_indicator_connecting()` | Orange inverted blink (`[tune]` 1800 ms on / 200 ms off) |
| MQTT connected | `display_mqtt_indicator_connected()` | Green steady on; orange off |
| MQTT error / reconnect backoff | `display_mqtt_indicator_error()` | Orange pattern: off 150 ms, off 150 ms, on 600 ms (loop) |
| MQTT inactive | `display_mqtt_indicator_off()` | Both bars off |

Transitions (derived in `mqtt_client.c`, applied in `app` on `EVT_MQTT_SESSION`):

| Condition | Indicator |
|-----------|-----------|
| Not armed, suspended, or Wi-Fi not ready | off |
| Broker session connected | connected |
| Reconnect backoff armed (`reconnect_at` in future) | error |
| Connect pending or in progress | connecting |
| Otherwise (armed, Wi-Fi up, not connected) | off |

Session lifecycle: [mqtt-protocol.md](mqtt-protocol.md) § Session display.

## Logical API (`display_presentation.h`)

Business code and UART CLI use `display_presentation_*` — never raw grid bytes
or pin masks.

### Steady state

| Function | Behavior |
|----------|----------|
| `display_presentation_set_digits(value)` | 0–999; leading-zero suppression on hundreds and tens; marks digits active |
| `display_presentation_clear_digits()` | Blanks grids 0–2 until the next `set_digits` |
| `display_presentation_set_unit(unit)` | `NONE`, `PERCENT`, or `GRAM` — lights unit pictograph on grid 3 |
| `display_presentation_icon_set(icon, on)` | Toggle one pictograph by `display_icon_t` label |
| `display_presentation_set_brightness(level)` | 1–4; default `[tune]` 4 |

Values above 999 are rejected with `PORT_ERR_INVALID_ARG`.

**Digits default:** After reset, grids 0–2 are **blank** — no implicit `0`. Digits
appear only after `set_digits` (e.g. idle weight scene sync or `display number`).
Digit/unit scene changes mark an internal dirty flag; `display_presentation_tick`
pushes them to TM1637 on the next `EVT_DISPLAY_TICK` (coalesced with blink/pattern
refreshes — no extra WFCI loan from the weight timer).
Icon-only updates (`icon_set`, `icon_blink`) do not change digit state.
`set_digits(0)` is explicit zero (`  0`), not the same as unset.

### Temporal effects

| Function | Behavior |
|----------|----------|
| `display_presentation_icon_blink(icon, on_ms, off_ms)` | Square-wave toggle; each duration 50–5000 ms; overrides steady visibility while active |
| `display_presentation_icon_blink_stop(icon)` | Cancel blink only; visibility reverts to steady `icon_set` state (does not force on) |
| `display_presentation_icon_pattern(icon, phases, count, loop)` | Multi-phase visibility sequence; each phase 50–5000 ms; up to 8 phases; cancels blink on that icon |
| `display_presentation_icon_pattern_stop(icon)` | Cancel pattern only; visibility reverts to steady `icon_set` state |
| `display_presentation_play_builtin(id, loop)` | Play built-in frame table (`ota`, `lock`) |
| `display_presentation_play_animation(anim, loop)` | Play caller-supplied frame table |
| `display_presentation_stop_animation()` | Restore composed steady scene |

Up to `[tune]` **4** icons may blink concurrently.

### Drive and power

| Function | Behavior |
|----------|----------|
| `display_presentation_tick(now_ms)` | Advance blink phases and animation; refresh when needed; returns ms until next tick (`UINT32_MAX` if idle) |
| `display_presentation_refresh()` | Compose scene; `try_show_grids` when available (presentation tick path), else blocking `show_grids` (CLI / one-shot boot paint) |
| `display_presentation_power_on/off()` | Wrap port power; first logical update auto-powers on |

`display_glyph.c` maps `display_icon_t` labels to grid 3/4 wire bits per
[display-tm1637.md](../10-hardware/components/display-tm1637.md).

### Steady state vs blink

- **Steady** (`icon_set`): resting on/off per icon; used when nothing temporal
  is overriding that icon.
- **Blink** (`icon_blink`): timed override; while active, the icon alternates
  visible/hidden regardless of steady state. `icon_blink_stop` ends the override
  only — the icon then shows steady on or steady off, whichever was last set.

`icon_blink_stop` is **not** `icon_set(..., true)`. UART: `display icon wifi
steady` vs `display icon wifi on` — see [uart-console.md](uart-console.md) §
Icon commands.

### Effect priority

1. **Animation active** — raw frame bytes replace the composed scene; blinks
   paused.
2. **No animation** — compose digits + unit + steady icons, then apply blink
   masks (icons in off-phase cleared from effective mask).
3. **Bench `display fill`** — bypasses presentation; does not alter scene
   state.

### Timing `[tune]`

| Parameter | Range | Default |
|-----------|-------|---------|
| Blink on/off | 50–5000 ms each | — |
| Wi-Fi connecting blink | 50–5000 ms each | 500 ms on / 500 ms off |
| Wi-Fi AP provisioning blink | 50–5000 ms each | 150 ms on / 150 ms off |
| MQTT connecting blink (orange bar) | 50–5000 ms each | 1800 ms on / 200 ms off |
| MQTT error pattern (orange bar) | 50–5000 ms per phase | off 150 ms, off 150 ms, on 600 ms (loop) |
| Presentation tick | 50 ms | FreeRTOS soft timer → `EVT_DISPLAY_TICK` |
| OTA animation frame period | 150 ms | — |
| Lock-busy animation frame period | 125 ms | — |

### WFCI refresh policy

Each physical frame is one `DISPLAY` profile bus loan (~1 ms). Presentation
tick and `refresh()` use `display_port.try_show_grids` when implemented: if
`try_acquire` fails, the frame is skipped and `scene_dirty` stays set for the
next tick — no multi-second blocking wait in presentation or indicator code.

Wi-Fi and MQTT indicator helpers (`display_wifi_indicator_*`,
`display_mqtt_indicator_*`) never call `show_grids` directly. Weight idle
sampling and TM1637 refresh share one `EVT_DISPLAY_TICK` handler turn in
`app` so a successful WFCI gap can update digits and icons together.

## Icon labels (`display_icon_t`)

| Label | CLI name |
|-------|----------|
| `DISPLAY_ICON_CHILD_LOCK` | `child_lock` |
| `DISPLAY_ICON_WIFI` | `wifi` |
| `DISPLAY_ICON_DISPENSING` | `dispensing` |
| `DISPLAY_ICON_PERCENT` | `percent` |
| `DISPLAY_ICON_GRAM` | `gram` |
| `DISPLAY_ICON_BLOCKAGE` | `blockage` |
| `DISPLAY_ICON_INSUFFICIENT_FOOD` | `insufficient_food` |
| `DISPLAY_ICON_BOWL_ERROR` | `bowl_error` |
| `DISPLAY_ICON_BAR_ORANGE` | `bar_orange` |
| `DISPLAY_ICON_BAR_GREEN` | `bar_green` |

## Built-in animations

Animations are opaque five-byte grid arrays (grids 0–4). No logical labels
inside frame data.

### OTA loading (`DISPLAY_ANIM_OTA`)

Six frames, `[tune]` 150 ms each. One outer segment lit per digit, phases
offset so the lit segment appears to chase clockwise around the three-digit
oval. Segment order per digit: A, B, C, D, E, F (`0x01`, `0x02`, `0x04`,
`0x08`, `0x10`, `0x20`). Grids 3–4 are `0x00` in every frame.

| Frame | Grid 0 | Grid 1 | Grid 2 |
|-------|--------|--------|--------|
| 0 | `0x01` | `0x04` | `0x10` |
| 1 | `0x02` | `0x08` | `0x20` |
| 2 | `0x04` | `0x10` | `0x01` |
| 3 | `0x08` | `0x20` | `0x02` |
| 4 | `0x10` | `0x01` | `0x04` |
| 5 | `0x20` | `0x02` | `0x08` |

## Display modes

| Mode | Content | When | Update rate |
|------|---------|------|-------------|
| **Weight** | Bowl grams when calibrated and sampled (e.g. `  42g`); uncalibrated → `---`; calibrated, no sample yet → blank digits + `g` | Default idle (hardcoded in `app` until `cmd/display` lands) | `[tune]` 500 ms (2 Hz) |
| **Eaten today** | Cumulative grams consumed since midnight (e.g. `  85g`) | User-selected idle alternative | `[tune]` 500 ms (2 Hz) |
| **Off** | All segments blank | Sleep or user preference | — |

**Eaten today** reads `eaten_today` from [weighing.md](weighing.md); resets at
local midnight per [monitoring.md](../20-stories/monitoring.md).

## Mode selection

- User sets idle mode via MQTT `cmd/display` (`weight`, `eaten_today`, `off`).
- Stored in NVDM `display/mode` (see [config-store.md](config-store.md)).
- No automatic mode override during dispense or fault — status pictographs
  on grid 3/4 may still assert (see [display-driver.md](display-driver.md)).
- On wake from sleep, resume the persisted mode after rail power-on.

## Brightness

- User-facing levels **1–4** (maps to TM1637 `0x88`–`0x8B`; see
  [display-driver.md](display-driver.md)).
- Configurable via MQTT `cmd/display`; default `[tune]` level 4 `[probe]`.
- Stored in NVDM `display/brightness`.

## Lock indicator

When child lock blocks a physical button gesture, blank the digit area and
unit for `[tune]` 1 s while `DISPLAY_ICON_CHILD_LOCK` blinks at `[tune]`
200 ms on / 200 ms off, then restore the active mode content (steady lock
icon remains if child lock is still active). The reset+dispense combo toggle
does **not** use this feedback — only the steady lock icon changes.

## Power policy

| Condition | Presentation action |
|-----------|---------------------|
| Enter sleep | Blank display; driver rail off (see [power-state-machine.md](power-state-machine.md)) |
| Exit sleep / boot | Rail on, re-init TM1637, resume persisted mode |
| Critical battery (< 5 %) | Blank display; rail off (see [battery-monitoring.md](battery-monitoring.md)) |
