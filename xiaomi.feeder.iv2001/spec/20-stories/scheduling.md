# Scheduling

## Overview

The user can define timed feeding schedules so the feeder dispenses
food automatically at the configured times.

## Schedule model

- Up to **32 time slots**.
- Each slot specifies: hour, minute, days-of-week bitmask, gram amount
  (5–150 g), and an enabled flag.
- **Hour + minute** uniquely identify a slot — only one feed per
  time-of-day.
- Slots can be created, updated, listed, and deleted via MQTT.

## Time source

- **Primary:** NTP over Wi-Fi, synced periodically.
- **Fallback:** internal RTC keeps time across short power losses.
- If the device has never synced (no Wi-Fi since boot), scheduled feeds
  are deferred until a valid time is acquired.

## Timezone

- Schedule times are **local civil time** (what the user sees on a clock).
- The device stores a compact DST rule, not a full timezone database.
- The user's dashboard or integration translates their IANA timezone into
  that rule and keeps it up to date when DST legislation changes.
- Defaults to UTC until configured.

## Persistence

- All schedule slots survive power cycles; stored in non-volatile
  config.
- A slot that has already fired in its current minute will not re-fire
  if the device reboots within that same minute.

## Next-feed reporting

The feeder publishes the next upcoming feed time and gram amount so the
user can see it in their dashboard or automation.
