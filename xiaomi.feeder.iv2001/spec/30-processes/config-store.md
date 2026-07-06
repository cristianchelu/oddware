# Configuration store

serves:
  - ../20-stories/connectivity.md
  - ../20-stories/provisioning.md
  - ../20-stories/scheduling.md

## Storage backend

MT7682 NVDM (Non-Volatile Data Management) — SDK-provided key-value flash store.

## Namespace / key table

| Namespace | Key | Type | Default | Description |
|-----------|-----|------|---------|-------------|
| wifi | ssid | string | — | Wi-Fi SSID (validation: [uart-console.md](uart-console.md#wi-fi-credential-rules)) |
| wifi | pass | string | — | Wi-Fi password; key optional for open networks (validation: [uart-console.md](uart-console.md#wi-fi-credential-rules)) |
| mqtt | host | string | — | Broker hostname / IP |
| mqtt | port | uint16 | 1883 | Broker port |
| mqtt | user | string | "" | Username (empty = anonymous) |
| mqtt | pass | string | "" | Password |
| mqtt | device_id | string | (empty) | MQTT topic device ID; empty = last 6 hex chars of STA MAC (see [mqtt-protocol.md](mqtt-protocol.md)) |
| mqtt | tls | bool | false | Enable TLS; `true` rejected at `mqtt_cred_load` until TLS adapter exists |
| feed | mode | enum | open_loop | Dispense mode: open_loop / compensated |
| feed | default_g | uint8 | 10 | Manual button portion grams |
| feed | child_lock | bool | false | All physical button gestures blocked except unlock combo |
| feed | overfill_enabled | bool | false | Skip scheduled feeds when bowl mass known and ≥ threshold |
| feed | overfill_threshold_g | uint8 | 50 | Overfill threshold in grams; clamped 30–100 on load |
| display | mode | enum | weight | Display mode: weight / eaten_today / off |
| display | brightness | uint8 | 4 | TM1637 brightness (1–4, maps to `0x88`–`0x8B`) |
| schedule | enabled | bool | true | Global schedule master switch |
| schedule | slots | blob (IF1S v1) | empty | Packed slot config (up to 32 slots) — see [scheduler-engine.md](scheduler-engine.md) § NVDM persistence |
| schedule | runtime | blob (IF1R v1) | — | **Future** — today runtime snapshot; separate key from config |
| time | tz_rule | string | UTC0 | POSIX TZ string (see `scheduler-engine.md`); key absent → UTC0 |
| time | tz_label | string | "" | IANA name for display only; not used by scheduler; key absent → `""` |
| calib | zero | int32 | — | Raw CS1270 count with bowl removed |
| calib | span_g | int32 | 350 | Provided bowl mass in grams (`[product]`) |
| calib | span_raw | int32 | — | Raw CS1270 count at span_g |
| power | battery_wifi | enum | on | Wi-Fi on battery: on / off / scheduled_only |
| power | batt_scale_x1000 | uint16 | 11000 | Pack mV = pin_mV × value / 1000; key absent → 11.0 default |
| power | batt_chemistry | uint8 | 0 | `battery_chemistry_t` enum; absent → `BATTERY_CHEM_AA_ALK_4S` (0); not loaded at runtime yet |
| system | boot_count | uint32 | 0 | Incremented each boot while the A/B control block `unverified` flag is set; cleared on crash-free slot confirm (see [ota-flow.md](ota-flow.md) § Slot health) |
| system | last_reset | enum | — | Reason for last reset (watchdog / ota / user / power) |

Firmware constants in `firmware/inc/config_keys.h`:

| Constant | Value |
|----------|-------|
| `CONFIG_GROUP_FEED` | `feed` |
| `CONFIG_KEY_FEED_MODE` | `mode` |
| `CONFIG_KEY_CHILD_LOCK` | `child_lock` |
| `CONFIG_KEY_OVERFILL_ENABLED` | `overfill_enabled` |
| `CONFIG_KEY_OVERFILL_THRESHOLD_G` | `overfill_threshold_g` |
| `CONFIG_GROUP_TIME` | `time` |
| `CONFIG_KEY_TZ_RULE` | `tz_rule` |
| `CONFIG_KEY_TZ_LABEL` | `tz_label` |
| `CONFIG_GROUP_SCHEDULE` | `schedule` |
| `CONFIG_KEY_SCHEDULE_ENABLED` | `enabled` |
| `CONFIG_KEY_SCHEDULE_SLOTS` | `slots` |
| `CONFIG_KEY_SCHEDULE_RUNTIME` | `runtime` |

Schedule runtime status (`state`, `skip_today`, `g_actual`, `fired_today`,
`today_enabled`) is **RAM only** in current firmware. A future `schedule/runtime`
(IF1R) blob may persist a subset across reboot — see
[scheduler-engine.md](scheduler-engine.md) § Runtime state (RAM only).

## Access pattern

| Phase | Behavior |
|-------|----------|
| Boot | Read all keys → populate runtime config struct (`tz_rule_init` parses POSIX `time/tz_rule` into RAM) |
| Runtime | Write on change (from MQTT entity commands, provisioning, calibration, schedule update) |
| Runtime (`time/tz_rule`, `time/tz_label`) | UART `time set` and legacy MQTT `cmd/config` both call `time_config_apply` |
| Runtime (`feed/mode`) | UART `feed mode`, MQTT `cmd/feed/mode`, and `feed_config_mode_set` |
| Runtime (`feed/overfill_*`) | UART `feed overfill`, MQTT `cmd/feed/overfill`, web `/api/feed/overfill` |
| Clear `time/tz_label` | Empty string via UART or MQTT erases the key; load returns `""` |
| Clear `time/tz_rule` | Empty string via UART or MQTT erases the key; load returns `UTC0`; RAM cache refreshed immediately |
| Write discipline | Minimize write frequency — flash has limited erase cycles. Batch writes where possible. |

Keys that rarely change (wifi, mqtt, calib) are written only on explicit user action.
Keys that change periodically (schedule/slots, system/boot_count) tolerate
moderate write frequency.

## Factory reset

1. Erase all NVDM namespaces (wifi, mqtt, feed, display, schedule, time,
   calib, power, system). A namespace with no keys is already clear —
   `erase_group` succeeds without error. Also clear the LinkIt SDK STA
   profile (`STA/Ssid` in NVDM) so a failed provisioning attempt does not
   leave a ghost SSID in the radio stack.
2. Reboot.
3. Device enters first-boot AP provisioning mode (no stored Wi-Fi credentials).

Triggered by: pin-hole long press (7 s) or MQTT `cmd/reboot` with
`{"factory_reset": true}`. `[design]`

## Storage budget

Total flash: 2 MB. Partition layout is our choice (custom bootloader). `[design]`

| Option | Pros | Cons |
|--------|------|------|
| **SDK NVDM API** | Simple, proven, SDK-managed wear leveling | Fixed sector allocation, limited metadata |
| **Filesystem (littlefs)** | Flexible, supports logs / calibration history | More code, own wear leveling |
| **Hybrid** | NVDM for small critical config; littlefs for bulk data (schedules, logs) | Two subsystems to maintain |

Allocate whatever the application and OTA strategy leave free. Exact
partition sizes defined in `../10-hardware/flash.md`. `[design]`

## MQTT sync

| Topic | Direction | Content |
|-------|-----------|---------|
| `.../config` | State (retained) | Time/TZ snapshot (`tz_rule`, `tz_label`, `time_synced`, `utc_epoch`) |
| `.../feed/mode` | State (retained) | Dispense mode: `open_loop` or `compensated` |
| `.../feed/overfill` | State (retained) | `{"enabled":false,"threshold_g":50}` |
| `.../cmd/config` | Command | **Legacy** time slice only (`tz_rule`, `tz_label`) — see [mqtt-protocol.md](mqtt-protocol.md) |
| `.../cmd/feed/mode` | Command | Same payloads as `.../feed/mode` state |

Not writable via MQTT: `wifi/*` (requires reprovisioning), `calib/*` (requires
calibration action), `power/batt_scale_x1000` (requires UART `adc cal`), `system/*`
(internal bookkeeping).
