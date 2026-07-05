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
| `buildScheduleSkipBody(hour, min, skip)` | `POST /api/schedule/skip` |
| `buildDispenseBody(g)` | `POST /api/dispense` |
| `buildScheduleEnableBody(enabled)` | `POST /api/schedule/enable` |
| `buildScheduleTodayBody(enabled)` | `POST /api/schedule/today` |
| `buildConfigTzBody(tz_rule)` | `POST /api/config` time slice |
| `formatNextFeed(next)` | Next-feed banner text |
| `formatStatusMessage(st)` | Status strip line from `GET /api/status` |
| `formatSlotDayLetters(repeat_days)` | One-letter column in the slot table |
| `parseApiResponse(text)` | JSON object or plain string (feed mode GET) |
| `apiContentType(body)` | `application/json` vs `text/plain` for POST |
| `mutationMessage(r, okText, failPrefix?)` | User-visible result from `{ok, error?}` |

## Client validation

| Rule | When | Message |
|------|------|---------|
| At least one weekday | Save slot | `Pick at least one day` |

Server validation and NVDM writes are unchanged; failed POSTs show `error` from
the JSON response in the status strip (`ok` / `err` CSS classes).

## Test scope

| Layer | Command | Covers |
|-------|---------|--------|
| HTTP API / schedule commands | `make test-host` | Routes, firmware JSON handlers, `schedule.c` |
| Client logic | `make test-web` | Masks, POST body shapes, formatters, `parseApiResponse` |
| Browser / device | Manual | Layout, `fetch` against live device, polling |

Host client tests use Node built-in `node:test` only (no npm). See
[build-integration.md](../40-architecture/build-integration.md).

## Build

`logic.mjs` is not served separately. `build.sh` substitutes the
`<!-- INJECT_LOGIC -->` marker in `index.html` with stripped `logic.mjs` before
`gzip -9`. Output: gitignored `firmware/src/web_ui_gz.c`.
