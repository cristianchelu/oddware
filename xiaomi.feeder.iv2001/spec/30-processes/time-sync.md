# Time sync and civil clock

serves:
  - ../20-stories/scheduling.md
  - ../20-stories/monitoring.md

## Role

Provides UTC from NTP + RTC and local civil time from a compact DST rule.
The scheduler and eaten-today midnight reset consume `time_local_now()`;
this module does not execute schedule slots.

Timezone rule format and NVDM keys: [scheduler-engine.md](scheduler-engine.md)
§ Local timezone rule. MQTT config fields: [mqtt-protocol.md](mqtt-protocol.md)
§ Config snapshot.

## NTP sync

- **Primary:** NTP over Wi-Fi via SNTP (`pool.ntp.org`). `[design]`
- **First sync:** requested when STA reaches DHCP ready
  (`EVT_WIFI_STA_READY` → `time_sync_on_wifi_ready()`).
- **Periodic re-sync:** every `[tune]` 6 hours while Wi-Fi stays associated
  (`time_sync_poll()` on `EVT_TIMER_TICK`).
- **Manual:** `time sync` UART command (`time_sync_request_now()`).
- **Retry after failure:** while `time_sync_is_valid()` is false and Wi-Fi
  stays associated, a failed sync (boot, retry, or manual) schedules the next
  attempt after `[tune]` 60 s (`time_sync_poll()` on `EVT_TIMER_TICK`).
- Boot, periodic resync, manual, and retry attempts all call
  `time_sync_start()`;
  completion is handled once in `time_sync_on_finish()` (`time sync ok` /
  `time sync failed` on UART).
- **Task context:** SNTP start and completion polling run from the `app` task
  only — never from `wifi_sta`, `mqtt_io`, or MQTT callbacks.

On successful sync, MediaTek SNTP middleware writes UTC to the MT7682 RTC
(`hal_rtc_set_time()` inside `middleware/third_party/sntp`). `time_sync` then
reads the RTC back via `time_port` and sets `time_sync_is_valid()` true for
the remainder of the boot. Firmware does **not** write the RTC again after
SNTP — see SDK example `project/mt7682_hdk/apps/sntp_client`.

## On-chip RTC (no backup power)

- IV2001 does **not** connect MT7682 RTC backup (`VBAT`) — time is **not**
  retained across a full power cycle.
- While main power is on, the RTC free-runs between NTP resyncs (boot, 6 h,
  or manual `time sync`).
- After power-on, RTC contents are unreliable until the first NTP success
  this boot; `time_sync_is_valid()` stays false until then.

## Time-unknown deferral

Until the first successful NTP sync **since boot**:

- `time_sync_is_valid()` returns false.
- `time_local_now()` returns false.
- The scheduler must not fire slots (see scheduler-engine).
- Retained `.../config` publishes `time_synced: false` and `utc_epoch: 0`.

Scheduling and eaten-today midnight logic resume once NTP succeeds.

## Local civil time

`time_local_now()` returns broken-down local time:

| Field | Range | Notes |
|-------|-------|-------|
| `year` | 2020–2099 | Full year |
| `month` | 1–12 | |
| `day` | 1–31 | |
| `hour` | 0–23 | |
| `min` | 0–59 | |
| `sec` | 0–59 | |
| `wday_mon0` | 0–6 | 0 = Monday … 6 = Sunday (schedule `days` bitmask) |

Computation: `local = UTC + tz_rule_effective_offset_min(utc)` using the
RAM rule parsed from NVDM `time/tz_rule` at boot (`tz_rule_init`). Fall-back
hour during DST end uses the **first** occurrence of a repeated local
`(hour, minute)` (standard offset). Epoch ↔ civil-date math is shared in
`epoch_calendar.c` (RTC adapter, `time_local`, and `tz_rule` DST boundaries).

## MQTT and UART

- Retained `.../config` carries `tz_rule`, `tz_label`, `time_synced`,
  `utc_epoch` — see mqtt-protocol.
- `cmd/config` accepts `tz_rule` and `tz_label` in this slice.
- UART bench commands: `time show`, `time sync`, `time set tz_rule|tz_label` —
  see uart-console.md.
- UART `time set` and MQTT `cmd/config` use the same validation, NVDM write,
  and config-snapshot publish path (`time_config_apply`).

## Boot init

`time_sync_init()` initializes the RTC port (`hal_rtc_init()`). `tz_rule_init()`
loads and parses POSIX `time/tz_rule` into RAM. Both are called from
`app_start()` before the event loop runs.
