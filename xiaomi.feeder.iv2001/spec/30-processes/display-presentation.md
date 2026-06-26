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
| `display_ota_indicator.c` | OTA digit progress policy (connect blink, download bar, verify) |
| `display_dispense_indicator.c` | Dispensing pictograph policy (active blink, idle off) |
| `display_bowl_error_indicator.c` | Food-bowl pictograph policy (cal blink, bowl-missing steady) |
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

## Bowl error indicator

`display_bowl_error_indicator.c` drives `DISPLAY_ICON_BOWL_ERROR`. The `app`
task calls `display_bowl_error_indicator_sync()` from
`app_weight_sync_display_scene()` on each weight sample tick (same path as
digit updates). User-facing semantics:
[display.md](../20-stories/display.md) § Food bowl error. Bowl presence
threshold: [weighing.md](weighing.md) § Bowl presence.

| Condition | Presentation | `[tune]` blink on/off |
|-----------|--------------|----------------------|
| Uncalibrated / idle cal | `display_bowl_error_indicator_sync(BOWL_ERROR_CAL_INCOMPLETE)` | 600 ms / 600 ms |
| Zero done, span pending | `display_bowl_error_indicator_sync(BOWL_ERROR_CAL_SPAN_PENDING)` | 200 ms / 200 ms |
| Calibrated, bowl OK or no sample | `display_bowl_error_indicator_sync(BOWL_ERROR_NONE)` | steady off |
| Calibrated, bowl missing | `display_bowl_error_indicator_sync(BOWL_ERROR_BOWL_MISSING)` | steady on |

Grid 4 bit `0x04` lights the pictograph. Blink cadences use the same
`icon_blink` / `icon_blink_stop` pattern as the Wi-Fi indicator.

## Dispensing indicator

`display_dispense_indicator.c` drives `DISPLAY_ICON_DISPENSING`. The dispense
supervisor calls `display_dispense_indicator_active()` when a job is accepted
and `display_dispense_indicator_idle()` when the job completes or faults.
User-facing semantics: [display.md](../20-stories/display.md) § Dispensing
indicator.

| State | Presentation | `[tune]` blink on/off |
|-------|--------------|----------------------|
| Dispense job active | `display_dispense_indicator_active()` | 150 ms / 150 ms |
| Idle (no job) | `display_dispense_indicator_idle()` | steady off |

`display_dispense_indicator_active()` returns `PORT_ERR_BUSY` when all blink
slots are in use; the dispense supervisor logs and continues the job without
the pictograph. On success it calls `display_presentation_refresh()` so the
first on-phase is visible before the next `EVT_DISPLAY_TICK`.
`display_dispense_indicator_idle()` also refreshes immediately.

In **compensated** gram dispense, the job (and blink) remain active through
post-motor weigh settle until bowl grams are stabilized and the display is
updated — see [weight-compensation.md](weight-compensation.md).

## MQTT indicator (status lightbar)

`display_mqtt_indicator.c` drives `DISPLAY_ICON_BAR_ORANGE` and
`DISPLAY_ICON_BAR_GREEN`. `mqtt_client_request_connect()` and
`mqtt_client_step()` post `EVT_MQTT_SESSION`; the `app` task calls the
indicator helpers. Helpers update presentation **scene state only** — they do
not call `display_presentation_refresh()`. Physical TM1637 updates come from
`EVT_DISPLAY_TICK` via `display_presentation_tick` → `try_show_grids`. See
[app-event-loop.md](app-event-loop.md) § Coexistence with MQTT connect.

**Immediate refresh exceptions:** `display_dispense_indicator_active()` /
`idle()` and child-lock blocked feedback call `display_presentation_refresh()`
after scene changes so user-visible feedback is not delayed until the next
display tick.
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

## OTA indicator

`display_ota_indicator.c` drives digit grids 0–2 during an OTA session via
`display_presentation_ota_show` / `display_presentation_ota_stop`. The `app`
task does not poll OTA display state — `ota_client.c` calls the indicator on
MQTT command accept and on each `ota_progress_t` callback. User-facing
context: [updates.md](../20-stories/updates.md) § Panel progress. OTA
lifecycle: [ota-flow.md](ota-flow.md).

While OTA override is active, **all pictographs except Wi-Fi are forced off**
(bowl error, dispensing, gram/percent unit, child lock, MQTT bars, etc.).
Wi-Fi remains under `display_wifi_indicator_*` so link loss is still visible.

| Phase | When | Digit grids 0–2 | `[tune]` blink |
|-------|------|-----------------|----------------|
| **Connecting** | MQTT `cmd/ota` accepted → first HTTP body byte | G segment only (`0x40`), all three digits | 300 ms on / 300 ms off |
| **Downloading** | HTTP body streaming, `pct` 0–100 | Outer perimeter fills cumulatively; **no G** | — |
| **Verifying** | SHA-512 / flash verify after download | All ten outer segments lit + G | G 200 ms on / 200 ms off |
| **Applying** | Bank swap → reboot | OTA override cleared; normal composed scene until reboot | — |
| **Idle** | OTA error or post-boot MQTT idle | Override cleared; normal composed scene restored | — |

**Perimeter path** (ten steps, one per 10 % of download):

```text
Hundreds A → Tens A → Singles A → Singles B → Singles C → Singles D
  → Tens D → Hundreds D → Hundreds E → Hundreds F
```

Segment map (gfedcba): `A=0x01`, `B=0x02`, `C=0x04`, `D=0x08`, `E=0x10`,
`F=0x20`, `G=0x40`.

Cumulative fill at download `pct`: `filled = min(10, (pct + 9) / 10)` when
`pct > 0`, else `0` (one segment per 10 %, ceiling). `display_glyph_ota_bar(filled, g_on, out)` ORs path
segments into grids 0–2; grids 3–4 are always `0x00` in the helper (icons
composed separately on refresh).

Automatic OTA uses the **OTA presentation override** above. UART
`display anim ota` exercises the built-in chase animation (below), not live
download progress.

### Effect priority (updated)

1. **OTA override active** — OTA digit composition + Wi-Fi icon only; other
   icons suppressed; icon blinks paused for suppressed icons.
2. **Animation active** — raw frame bytes replace the composed scene; blinks
   paused.
3. **No OTA or animation** — compose digits + unit + steady icons, then apply
   blink masks.
4. **Bench `display fill`** — bypasses presentation; does not alter scene state.

## Logical API (`display_presentation.h`)

Business code and UART CLI use `display_presentation_*` — never raw grid bytes
or pin masks.

### Steady state

| Function | Behavior |
|----------|----------|
| `display_presentation_set_digits(value)` | 0–999; leading-zero suppression on hundreds and tens; marks digits active |
| `display_presentation_set_digits_dash()` | All three digit grids show segment dash (`0x40` each) — uncalibrated `---` |
| `display_presentation_set_digits_underflow()` | Grid 0 = dash (`0x40`); grids 1–2 blank — calibrated bowl-missing `-  g` |
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
| `display_presentation_ota_show(phase, pct)` | Enter OTA override (phase + download `pct` for bar fill) |
| `display_presentation_ota_stop()` | Clear OTA override; restore composed scene |

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

### Timing `[tune]`

| Parameter | Range | Default |
|-----------|-------|---------|
| Blink on/off | 50–5000 ms each | — |
| Wi-Fi connecting blink | 50–5000 ms each | 500 ms on / 500 ms off |
| Wi-Fi AP provisioning blink | 50–5000 ms each | 150 ms on / 150 ms off |
| Bowl error cal-incomplete blink | 50–5000 ms each | 600 ms on / 600 ms off |
| Bowl error span-pending blink | 50–5000 ms each | 200 ms on / 200 ms off |
| MQTT connecting blink (orange bar) | 50–5000 ms each | 1800 ms on / 200 ms off |
| MQTT error pattern (orange bar) | 50–5000 ms per phase | off 150 ms, off 150 ms, on 600 ms (loop) |
| Presentation tick | 50 ms | FreeRTOS soft timer → `EVT_DISPLAY_TICK` |
| OTA connecting G blink | 50–5000 ms each | 300 ms on / 300 ms off |
| OTA verifying G blink | 50–5000 ms each | 200 ms on / 200 ms off |
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

Ten frames, `[tune]` 150 ms each. One outer perimeter segment lit per frame
(follows the OTA download path in § OTA indicator), with segment **G steady
on** all three digit grids (`0x40` OR'd into each grid byte). Grids 3–4 are
`0x00` in every frame. Bench-only — automatic OTA uses the live OTA override.

| Frame | Segment | Grid 0 | Grid 1 | Grid 2 |
|-------|---------|--------|--------|--------|
| 0 | Hundreds A | `0x41` | `0x40` | `0x40` |
| 1 | Tens A | `0x40` | `0x41` | `0x40` |
| 2 | Singles A | `0x40` | `0x40` | `0x41` |
| 3 | Singles B | `0x40` | `0x40` | `0x42` |
| 4 | Singles C | `0x40` | `0x40` | `0x44` |
| 5 | Singles D | `0x40` | `0x40` | `0x48` |
| 6 | Tens D | `0x40` | `0x48` | `0x40` |
| 7 | Hundreds D | `0x48` | `0x40` | `0x40` |
| 8 | Hundreds E | `0x50` | `0x40` | `0x40` |
| 9 | Hundreds F | `0x60` | `0x40` | `0x40` |

## Display modes

| Mode | Content | When | Update rate |
|------|---------|------|-------------|
| **Weight** | Presented bowl mass when calibrated and sampled (auto-tare — see [auto-tare.md](auto-tare.md)): **whole grams rounded** from internal tenth-grams (e.g. 423 dg → `  42g`); uncalibrated → `---g`; calibrated bowl missing → `-  g`; calibrated, no sample yet → blank digits + `g` | Default idle (hardcoded in `app` until `cmd/display` lands) | `[tune]` 500 ms (2 Hz) |
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
