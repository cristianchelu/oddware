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
| `days` | uint8 | bitmask | bit 0 = Mon … bit 6 = Sun; 0x7F = every day |
| `grams` | uint8 | 5–150 | Portion size |

`hour` + `minute` form the slot key — at most one entry per time-of-day.
`cmd/schedule/set` upserts by key; `cmd/schedule/delete` addresses a slot
by `hour` and `min` only.

Serialized as blob in NVDM key `schedule/slots`.

## RTC tick and slot matching

- A periodic tick at `[tune]` 1-minute resolution checks all enabled slots.
- For each slot where `hour == current_hour && minute == current_minute`
  and `days` bitmask includes current weekday:
  - Check "fired today" flag for this slot.
  - If not fired: submit dispense request with `slot.grams`, set fired flag.
  - If already fired: skip (prevents re-trigger on same minute).

## "Fired today" tracking

- Each slot is keyed by `(hour, minute)`; an in-memory "fired today" flag
  per key (not persisted to flash).
- Flags reset at **midnight** (local time, per configured timezone rule).
- On reboot mid-day, all flags start cleared — slots that should have
  already fired will re-trigger if their minute hasn't passed. Acceptable
  trade-off vs. persisting fired state. `[design]`

## NVDM persistence

- Full schedule array written to NVDM on any add/update/delete.
- Read from NVDM at boot to populate runtime schedule table.
- Minimize writes: batch schedule changes from MQTT if multiple arrive
  within a short window. `[design]`

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

### Packed rule struct (firmware canonical form)

Fixed-size, packed binary — stored in NVDM key `time/tz_rule` (blob).
All multi-byte integers are little-endian.

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `std_offset_min` | int16 | −720…840 | Standard-time offset from UTC, minutes |
| `dst_offset_min` | int16 | −720…840 | Daylight offset from UTC, minutes |
| `start_m` | uint8 | 1–12 | DST start month |
| `start_w` | uint8 | 1–5 | Week of month (`5` = last week containing `start_d`) |
| `start_d` | uint8 | 0–6 | Day of week (`0` = Sunday) |
| `start_h` | uint8 | 0–23 | Transition hour, **local standard time** |
| `end_m` | uint8 | 1–12 | DST end month |
| `end_w` | uint8 | 1–5 | Week of month (`5` = last week containing `end_d`) |
| `end_d` | uint8 | 0–6 | Day of week (`0` = Sunday) |
| `end_h` | uint8 | 0–23 | Transition hour, **local standard time** |

When `dst_offset_min == std_offset_min`, DST is disabled — transition
fields are ignored. Default at factory reset: `std_offset_min = 0`, all
other fields zero (UTC, no DST).

Optional display-only IANA label in NVDM `time/tz_label` (string, max 47
bytes). The firmware never parses it; scheduling uses only `time/tz_rule`.

### Wire string (MQTT / logging)

MQTT config uses a canonical string form of the same struct (firmware
parses on `cmd/config`, serializes into retained `.../config`). Format:

```text
<std_min>/<dst_min>/<start>/<end>
```

Each transition is `M.w.d.h` (month.week.dow.hour), matching the struct
fields. **No DST:** `<std_min>` only (shorthand) or `<std_min>/<std_min>`.

Examples:

| Rule | Wire string |
|------|-------------|
| UTC | `0` |
| UTC+8, no DST | `480` |
| US Eastern | `-300/-240/3.2.0.2/11.1.0.2` |
| EU (CET/CEST) | `60/120/3.5.0.2/10.5.0.3` |

`cmd/config` accepts `{"tz_rule": "<wire string>"}` and optionally
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

| Topic | Direction | Payload |
|-------|-----------|---------|
| `.../schedule/list` | state (retained) | Full slot array |
| `.../cmd/schedule/set` | command | Single slot object |
| `.../cmd/schedule/delete` | command | `{"hour":H,"min":M}` |
| `.../schedule/next` | state (retained) | `{"hour":H,"min":M,"g":G,"in_min":N}` |
