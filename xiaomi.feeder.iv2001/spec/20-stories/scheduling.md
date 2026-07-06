# Scheduling

## Overview

The user can define timed feeding schedules so the feeder dispenses
food automatically at the configured times.

## Schedule model

- Up to **32 time slots**.
- Each slot specifies: hour, minute, weekdays, gram amount (5–150 g), and an
  enabled flag. MQTT exposes weekdays as a day list (`repeat_days`, Mon=0 …
  Sun=6); the bench UART uses a numeric weekday bitmask.
- **Hour + minute** uniquely identify a slot — only one feed per
  time-of-day.
- Slots can be created, updated, listed, and deleted via MQTT, the LAN web UI
  (see [web-ui.md](../30-processes/web-ui.md)), or the bench UART `schedule`
  commands (see [uart-console.md](../30-processes/uart-console.md)).
- A global master switch enables or disables the entire schedule.
- A today-only override can disable scheduled feeds for the current day
  without changing recurring slot config.

## Runtime status

Each slot reports a native status on MQTT (`pending`, `to_be_skipped`,
`skipped`, `skipped_full`, `dispensing`, `dispensed`, `failed`). The user can
skip an individual slot for today only. When a feed completes, actual
dispensed grams are reported alongside the target amount.

## Overfill protection

The user can enable **overfill protection** so scheduled feeds are skipped
when the bowl already holds at least a configured amount of food (30–100 g).
Manual dispenses (button, MQTT, web UI, UART) always bypass this check. When
a scheduled feed is skipped for overfill, the slot status is `skipped_full`.
Settings persist in non-volatile storage and are configurable from Home
Assistant, the LAN web UI Settings tab, MQTT, or UART.

Runtime status lives in RAM only — it resets at local midnight and after
reboot. Slot configuration persists in non-volatile storage.

## Time source

- **Primary:** NTP over Wi-Fi, synced periodically.
- **On-chip RTC** holds UTC only while powered; IV2001 has no RTC backup
  supply — every boot requires NTP before schedules run (see time-sync.md).
- If the device has never synced (no Wi-Fi since boot), scheduled feeds
  are deferred until a valid time is acquired.

## Timezone

- Schedule times are **local civil time** (what the user sees on a clock).
- The device stores a POSIX `TZ` string in config (parsed into a DST rule in
  RAM), not a full timezone database.
- The user's dashboard or integration translates their IANA timezone into
  that string and keeps it up to date when DST legislation changes.
- Defaults to UTC until configured.

## Persistence

- Schedule slot configuration and the global enable flag survive power
  cycles; stored in non-volatile config.
- Runtime status (per-slot state, skip-today, actual grams) does **not**
  persist — see [scheduler-engine.md](../30-processes/scheduler-engine.md).
- After reboot, past-due slots for today are marked skipped on the first
  scheduler tick once time is valid.

## Next-feed reporting

The feeder publishes the next upcoming feed time and gram amount so the
user can see it in their dashboard or automation.

## Home Assistant

A **Feeding schedule** binary sensor is auto-discovered. Entity state
reflects the global schedule on/off switch; the full schedule array and
runtime status appear as entity attributes.
