# LAN web UI client

serves:
  - ../20-stories/scheduling.md
  - ../20-stories/connectivity.md
  - ../20-stories/feeding.md

## Purpose

The admin panel is a single-page browser client over the HTTP API in
[web-ui.md](web-ui.md). POST bodies and read parsing match
[mqtt-protocol.md](mqtt-protocol.md). Server-side routing and validation are
tested in `test_web_api.c` and `test_schedule_cmd.c`; this document covers
**client-only** behavior in `tools/web/`.

## Source layout

| File | Role |
|------|------|
| `tools/web/index.html` | Markup, styles, DOM event wiring |
| `tools/web/logic.mjs` | Pure helpers (weekday masks, JSON bodies, formatters) |
| `tools/web/build.sh` | Inlines `logic.mjs` into the page, gzip, emit `web_ui_gz.c` |
| `tools/web/test_logic.mjs` | Host tests (`node --test`) |

`build.sh` strips `export` keywords so inlined functions are global inside the
page `<script>`. The flash image still receives one gzipped HTML file.

## Weekday model

| UI | Wire format |
|----|-------------|
| Checkboxes labelled Mon–Sun left to right | `repeat_days` integer array on `POST /api/schedule/set` |
| Index `0` = Monday, `6` = Sunday | Same as MQTT `cmd/schedule/set` and UART bitmask semantics |

The UI uses a 7-bit mask internally (`1 << day_index`). `maskToIndices(mask)`
produces the sorted `repeat_days` array for the POST body.

## Pure helpers (`logic.mjs`)

| Function | Use |
|----------|-----|
| `DAYS` | Weekday labels for the checkbox row |
| `indicesToMask(indices)` | Bitmask from selected indices |
| `maskToIndices(mask)` | `repeat_days` array from bitmask |
| `readMaskFromCheckedValues(values)` | Bitmask from checkbox `value` strings |
| `validateScheduleMask(mask)` | `{ok, error?}` — requires non-zero mask before save |
| `buildScheduleSetBody({hour, min, mask, g, enabled})` | `POST /api/schedule/set` object |
| `buildScheduleDeleteBody(hour, min)` | `POST /api/schedule/delete` |
| `buildScheduleToggleBody(hour, min)` | `POST /api/schedule/toggle` |
| `buildScheduleSkipBody(hour, min, skip)` | `POST /api/schedule/skip` |
| `buildDispenseBody(g)` | `POST /api/dispense` |
| `buildScheduleEnableBody(enabled)` | `POST /api/schedule/enable` |
| `buildScheduleTodayBody(enabled)` | `POST /api/schedule/today` |
| `buildConfigTzBody(tz_rule)` | `POST /api/config` time slice |
| `formatNextFeed(next)` | Next-feed summary line |
| `formatNextFeedParts(next)` | Two-line next feed (`Next feed · in N min` / `HH:MM · Ng`) |
| `formatDeviceTime(st)` | Feeder `local_time` in header (or `—` if unsynced) |
| `formatStatusAlerts(st)` | Problem badges (time sync, bowl error) |
| `statusStateRows(st)` | Current-state label/value rows for the Now tab |
| `formatBowlWeightWire(wire)` | MQTT `bowl_weight` string → display |
| `formatEditorTitle(time)` | Slot editor heading (`New feeding time` / `Edit HH:MM`) |
| `sortSlotsByTime(slots)` | Client-side table sort by clock time |
| `WEEKDAY_MASK` / `ALL_DAYS_MASK` | Day-chip presets in editor |
| `formatRefreshTime(date)` / `formatMetaLine(st, at)` | Host tests only; not shown in UI |
| `formatFeedModeLabel(mode)` | Human-readable feed mode (settings select uses same labels) |
| `formatSlotDayLetters(repeat_days)` | One-letter column in the slot table |
| `formatSlotClock(time)` | Zero-padded `HH:MM` for card column alignment |
| `weekdayFromLocalTime(local_time)` | Mon=0 … Sun=6 from feeder `local_time` date |
| `slotDayBadges(repeat_days, todayIndex?)` | Per-day `{letter, label, on, today}` for schedule cards |
| `formatSlotDays(repeat_days)` | Full weekday list (`title` on table cell) |
| `formatSlotState(state)` / `slotStateTone(state)` | Slot state badge |
| `parseApiResponse(text)` | JSON object or plain string (feed mode GET) |
| `apiContentType(body)` | `application/json` vs `text/plain` for POST |
| `mutationMessage(r, okText, failPrefix?)` | User-visible result from `{ok, error?}` |

## Page layout (frozen v0)

Sticky tab bar (**Now** · **Schedule** · **Settings**); one pane visible at a time
(CSS `:checked` on hidden radios — no framework).

### Now tab

| Block | Source |
|-------|--------|
| Next feed | `/api/status` `next` — two-line hero (`formatNextFeedParts`) |
| Problem alerts | `/api/status` — time not synced, `bowl_error` |
| **Current state** grid | `/api/status` telemetry fields — see below |
| Feed now | `POST /api/dispense` |

**Current state** rows (`statusStateRows`) expect these `/api/status` fields:

| Row label | Field | Display helper |
|-----------|-------|----------------|
| Food in bowl | `bowl_weight` | `formatBowlWeightWire` |
| Hopper | `hopper` | `formatHopperLevel` |
| Activity | `dispense_busy` | `formatBusy` |
| Bowl | `bowl_error` | `formatBowlHealth` |
| Battery | `battery` (optional) | `formatBatteryWire` |
| Power | `mains` (optional) | `formatMainsWire` |

Poll `/api/status` every 5 s; do not add per-topic GETs in v0.

### Schedule tab

| Block | Behavior |
|-------|----------|
| Master toggles | Automatic schedule / feeds today |
| Slot cards | Fixed grid: time · grams · day letters · status (right) |
| Day letters | Plain when scheduled; **today** gets bordered on/off badge (`slotDayBadges`) |
| Row actions | Disable/Enable, Skip today, Delete (44px tap targets) |
| Add/edit form | Always visible; Save upserts by time; Clear resets |

### Settings tab

Feed mode + timezone (collapsed section).

### Chrome

Header: title · feeder `local_time` (`formatDeviceTime`) · refresh control.

## Client validation

| Rule | When | Message |
|------|------|---------|
| At least one weekday | Save slot | `Pick at least one day` |

Server validation and NVDM writes are unchanged; failed POSTs show `error` from
the JSON response in the toast (`ok` / `err` classes).

## Test scope

| Layer | Command | Covers |
|-------|---------|--------|
| HTTP API / schedule commands | `make test-host` | Routes, firmware JSON handlers, `schedule.c` |
| Client logic | `make test-web` | Masks, POST body shapes, formatters, `parseApiResponse` |
| Browser / device | `make preview-web` or manual | Layout, `fetch` against mock or live device |

Host client tests use Node built-in `node:test` only (no npm). Local UI preview:
`make preview-web` serves the bundled page with an in-memory mock API at
`http://127.0.0.1:8765/` (`tools/web/dev_server.mjs`, `mock_api.mjs`). See
[build-integration.md](../40-architecture/build-integration.md).

## Build

`logic.mjs` is not served separately. `build.sh` substitutes the
`<!-- INJECT_LOGIC -->` marker in `index.html` with stripped `logic.mjs` before
`gzip -9`. Output: gitignored `firmware/src/web_ui_gz.c`.
