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
| `display_presentation.c` | Future idle modes, periodic refresh |
| MQTT `cmd/display` handler | Future mode/brightness commands |

**Dependency rule:** presentation modules call `display_port` only. They must
not include TM1637 command bytes, grid indices, AW9523B registers, or SDK
I2C/GPIO headers. How the panel is driven:
[display-driver.md](display-driver.md).

**Boot self-test:** On power-on, show all segments lit (`0xFF` on grids 0–4)
for `[tune]` 1000 ms, then black. Driver provides `show_fill` / `blank`; policy
lives here and in `display_boot.c`. Hardware steps:
[display-driver.md](display-driver.md) § Boot self-test.

**Wi-Fi connecting blinker (future):** Toggle the Wi-Fi pictograph (grid 3
bit `0x02`) from the presentation layer via `set_icons`, driven by Wi-Fi port
state — not in the display driver.

## Display modes

| Mode | Content | When | Update rate |
|------|---------|------|-------------|
| **Weight** | Current bowl weight in grams (e.g. `  42g`) | Default idle | `[tune]` every 2 s |
| **Eaten today** | Cumulative grams consumed since midnight (e.g. `  85g`) | User-selected idle alternative | `[tune]` every 2 s |
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

When child lock blocks a button press, show the busy/lock spinner pattern
(multi-frame digit animation in [display-driver.md](display-driver.md)) for
`[tune]` ~1 s, then restore the active mode content.

## Power policy

| Condition | Presentation action |
|-----------|---------------------|
| Enter sleep | Blank display; driver rail off (see [power-state-machine.md](power-state-machine.md)) |
| Exit sleep / boot | Rail on, re-init TM1637, resume persisted mode |
| Critical battery (< 5 %) | Blank display; rail off (see [battery-monitoring.md](battery-monitoring.md)) |
