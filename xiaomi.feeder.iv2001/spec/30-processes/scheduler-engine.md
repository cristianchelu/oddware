# Scheduler engine

serves:
  - ../20-stories/scheduling.md

## Schedule model

Up to `[tune]` 32 time slots. Each slot:

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `enabled` | bool | — | Slot active or suspended |
| `hour` | uint8 | 0–23 | Trigger hour (local time) |
| `minute` | uint8 | 0–59 | Trigger minute |
| `days` | uint8 | bitmask | bit 0 = Mon … bit 6 = Sun; 0x7F = every day; `0` = never repeat |
| `grams` | uint8 | 5–150 | Portion size |

`hour` + `minute` form the slot key — at most one entry per time-of-day.
`cmd/schedule/set` upserts by key; `cmd/schedule/delete` addresses a slot
by `hour` and `min` only.

MQTT and `schedule/state` use `repeat_days` (array Mon=0 … Sun=6). The
firmware stores weekdays internally as a `uint8_t` bitmask (`days` field in
RAM and NVDM). UART `schedule set` accepts the bitmask decimal form.

## NVDM persistence

| Key | Type | Default | Contents |
|-----|------|---------|----------|
| `schedule/enabled` | bool string | `true` | Global master switch (`"0"` / `"1"`) |
| `schedule/slots` | blob (IF1S v1) | empty | Packed slot config — see below |
| `schedule/runtime` | blob (IF1R v1) | — | **Future** — today runtime snapshot; not loaded yet |

Config and runtime use **separate keys** so dispense-driven runtime writes
(future) do not rewrite the slot config blob. `[design]`

### `schedule/slots` (IF1S v1)

Fixed-size binary blob (magic `IF1S`, version 1). Header plus up to 32 slots,
each: `hour`, `min`, `days` (bitmask), `g`, `flags` (bit0 = enabled).
Worst case ~168 B. Missing key or zero-length blob → empty schedule in RAM.
Corrupt blob (wrong size, magic, version, slot fields, or duplicate
`(hour, min)` keys) → empty schedule in RAM and immediate rewrite of a
valid empty IF1S blob to NVDM. `[design]`

NVDM writes on user-facing config changes (CRUD). Typical rate: ~0–2 writes
per week. Global enable uses `schedule/enabled` only — does not rewrite
`schedule/slots`.

### `schedule/runtime` (IF1R v1, future)

Reserved layout for optional reboot continuity of today-only state
(`state`, `g_actual`, `fired_today`, `today_enabled`). `skip_today` still
clears at local midnight even when persisted. Config CRUD erases or
invalidates the runtime blob. Not implemented in firmware yet — runtime
remains RAM-only until a future change.

## Runtime state (RAM only)

Runtime fields are included in retained MQTT `schedule/state` and cleared at
local midnight. They are **not** written to NVDM in the current firmware.

| Field | Scope | Cleared |
|-------|-------|---------|
| `state` | per slot | local midnight |
| `skip_today` | per slot | local midnight |
| `g_actual` | per slot | local midnight |
| `fired_today` | per slot | local midnight |
| `today_enabled` | global | local midnight |

After reboot or power loss all runtime fields start at defaults
(`state=pending`, `skip_today=false`, `g_actual` unknown, `fired_today=false`,
`today_enabled=true`). Retained `schedule/state` republishes on MQTT connect
with rebuilt runtime view. Past-due slots for today are marked `skipped` on
the first `schedule_poll` after `time_sync_is_valid()`.

## Status state machine

Device publishes native `state` per entry in MQTT. `disabled` is **not**
published — consumers derive it when `enabled=false` and the row is still
future-today.

| Native `state` | Set when |
|----------------|----------|
| `pending` | Applies today, not skipped, not yet dispensed this cycle |
| `to_be_skipped` | `skip_today=true` and dispense time is still in the future |
| `skipped` | `skip_today=true` and time passed; OR due minute missed; OR `today_enabled=false` for a future today entry |
| `skipped_full` | Overfill protection enabled, bowl mass known, and `bowl_g >= overfill_threshold_g` at fire time — no dispense job submitted |
| `dispensing` | Gram job submitted for this slot and dispense supervisor active |
| `dispensed` | Terminal dispense event with `event_type=success` or `underfill` |
| `failed` | Terminal event with `event_type` in `stuck`, `empty_hopper`, `aborted` |

**Global `schedule/enabled=false`:** entries keep native state; UI layers
mark future rows disabled.

**`g_actual`:** set from dispense completion `grams` when `source=schedule`
and slot key matches active slot; published until midnight reset.

## RTC tick and slot matching

- `schedule_poll()` runs on `EVT_TIMER_TICK` when the local civil minute
  changes (`[tune]` 1-minute resolution).
- Slots run only when `time_sync_is_valid()` is true.
- For each slot where `enabled && global_enabled && days` includes current
  weekday, `!skip_today`, and `today_enabled`:
  - If `fired_today`, skip.
  - If `hour == current_hour && minute == current_minute`: invoke the
    registered **fire callback** (app task submits `dispense_submit_grams`
    with `DISPENSE_SOURCE_SCHEDULE`).
  - On `SCHEDULE_FIRE_OK`: set `fired_today`, `state=dispensing`, record
    active slot key.
  - On `SCHEDULE_FIRE_SKIPPED_FULL`: set `fired_today`, `state=skipped_full`
    (terminal; no dispense job).
  - On `SCHEDULE_FIRE_BUSY` or `SCHEDULE_FIRE_REJECTED`: leave `fired_today`
    clear; mark `skipped` on minute advance if never fired.

### Overfill protection at fire time

When NVDM `feed/overfill_enabled` is true, the fire callback evaluates bowl
mass before submitting a schedule dispense:

1. Read cached bowl sample via `app_bowl_dg_snapshot()` and
   `bowl_mass_present_dg()` (same known/unknown rules as `.../bowl_weight`).
2. If mass is **not** known, proceed as a normal scheduled feed (submit
   dispense).
3. If `display_g >= feed/overfill_threshold_g` (integer grams, `>=`), return
   `SCHEDULE_FIRE_SKIPPED_FULL`.
4. Otherwise submit dispense as today.

Manual dispense paths (`button`, `MQTT`, web, UART) do not run this check.
No `dispense/event` is published for `skipped_full` — status appears in
retained `schedule/state` only. `skipped_full` is terminal for the rest of the
local day (`fired_today` set, same as `dispensed` / `failed`). Runtime slot
state is not persisted: after reboot, past-due slots reconcile to generic
`skipped` per the rules below.
- If a due slot never fires during its minute (busy queue, time invalid),
  the first poll of the next minute sets `state=skipped`.

## "Fired today" tracking

- Each slot is keyed by `(hour, minute)`; an in-memory "fired today" flag
  per key (not persisted to flash).
- Flags reset at **midnight** (local time, per configured timezone rule).
- On reboot mid-day, all flags start cleared — first `schedule_poll`
  reconciles past-due rows to `skipped`.

## Time source

NTP sync, on-chip RTC, and time-unknown deferral are defined in
[time-sync.md](time-sync.md). Summary for scheduling:

- Slots run only when `time_sync_is_valid()` is true (first NTP success since
  boot).
- Local slot matching uses `time_local_now()` (civil time from TZ rule).
- Retained `.../config` exposes `time_synced` and `utc_epoch` instead of a
  separate time topic.

## Local timezone rule

The device has **no IANA timezone database**. Schedules are stored and
matched in **local civil time**. UTC comes from NTP/RTC; local time is
`UTC + effective_offset(utc_now)` where `effective_offset` is derived from
a compact DST rule.

The integration layer (Home Assistant, dashboard, etc.) owns the IANA TZ
data and pushes **current** transition rules to the feeder whenever the
user changes timezone or when upstream TZ data changes. `[design]`

### Runtime rule (RAM)

Parsed from NVDM at boot and refreshed on `time set` / `cmd/config`.
Used by `tz_rule_effective_offset_min()` — never re-read from NVDM on the
scheduler or `time_local_now` hot path.

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `std_offset_min` | int16 | −720…840 | Standard-time offset from UTC, minutes east |
| `dst_offset_min` | int16 | −720…840 | Daylight offset from UTC, minutes east |
| `start_m` | uint8 | 1–12 | DST start month |
| `start_w` | uint8 | 1–5 | Week of month (`5` = last week containing `start_d`) |
| `start_d` | uint8 | 0–6 | Day of week (`0` = Sunday) |
| `start_h` | uint8 | 0–23 | Transition hour, **local standard time** |
| `end_m` | uint8 | 1–12 | DST end month |
| `end_w` | uint8 | 1–5 | Week of month (`5` = last week containing `end_d`) |
| `end_d` | uint8 | 0–6 | Day of week (`0` = Sunday) |
| `end_h` | uint8 | 0–23 | Transition hour, **local standard time** |

When `dst_offset_min == std_offset_min`, DST is disabled — transition
fields are ignored.

### NVDM storage (`time/tz_rule`)

POSIX `TZ` environment-variable string (max **80** UTF-8 bytes). Stored
verbatim after validation — no packed blob, no custom numeric wire format.
Default at factory reset: `UTC0`.

UART `time show` and retained `.../config` echo the stored string.

Optional display-only IANA label in NVDM `time/tz_label` (string, max 47
bytes). The firmware never resolves zone names; scheduling uses only
`time/tz_rule`.

### POSIX subset (set path)

Accepted on `time set tz_rule`, MQTT `cmd/config`, and boot cache parse.
Rejected otherwise (including legacy numeric wire such as `480`).

| Supported | Example |
|-----------|---------|
| Fixed offset | `UTC0`, `EET-2`, `IST-5:30` |
| US Eastern | `EST5EDT,M3.2.0,M11.1.0` |
| EU / Bucharest | `EET-2EEST,M3.5.0/3,M10.5.0/4` |

Reject: `J` Julian rules, abbreviation-only strings (`EEST`), strings
over 80 bytes, transition times with fractional hours (`M3.2.0/0:01`).

Offset fields accept optional `:mm` and `:mm:ss` (`IST-5:30`, `NPT-5:45`).
POSIX offsets are west-positive; firmware converts to minutes east for the
RAM struct.

Transition `M` rules use POSIX `Mm.w.d` with optional `/h` (default **2**
when omitted). DST name without explicit offset defaults to std + 1 hour
local (POSIX default).

`cmd/config` accepts `{"tz_rule": "<POSIX string>"}` and optionally
`{"tz_label": "Europe/Bucharest"}` for dashboards.

### Offset evaluation

- Given UTC epoch, determine whether UTC falls in the DST window defined
  by the start/end transitions (POSIX `M.w.d` semantics for locating the
  transition instant each year).
- Return `dst_offset_min` inside the window, else `std_offset_min`.
- Evaluated once per scheduler tick and when computing `schedule/next`.

### Fall-back hour policy

On the repeated local hour when clocks go back, a slot fires **at most
once** — the first time that `(hour, minute)` is reached. `[design]`

### Limitations

- One DST period per year (covers US, EU, UK, and most regions).
- Encodes **current** annual rules only — no historical TZ changes.
- Exotic zones that do not fit this model must be approximated by the
  integration layer or configured as a fixed offset.

## MQTT interface

Operator mutations are accepted on MQTT command topics and on the bench UART
`schedule` command tree ([uart-console.md](uart-console.md) § `schedule` commands).

See [mqtt-protocol.md](mqtt-protocol.md) § Schedule for full topic table,
`schedule/state` schema, and command payloads.

| Topic | Direction | Payload |
|-------|-----------|---------|
| `.../schedule/state` | state (retained) | Full schedule document with runtime status |
| `.../schedule/next` | state (retained) | `{"hour":H,"min":M,"g":G,"in_min":N}` |
| `.../cmd/schedule/set` | command | Single slot object |
| `.../cmd/schedule/delete` | command | `{"hour":H,"min":M}` |
| `.../cmd/schedule/toggle` | command | `{"hour":H,"min":M}` |
| `.../cmd/schedule/skip` | command | `{"hour":H,"min":M,"skip":true}` |
| `.../cmd/schedule/enable` | command | `{"enabled":true}` |
| `.../cmd/schedule/today` | command | `{"enabled":true}` |
