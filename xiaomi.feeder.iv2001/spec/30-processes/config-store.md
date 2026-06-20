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
| display | mode | enum | weight | Display mode: weight / eaten_today / off |
| display | brightness | uint8 | 4 | TM1637 brightness (1–4, maps to `0x88`–`0x8B`) |
| schedule | slots | blob | [] | Serialized schedule array (up to 32 slots) |
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
| `CONFIG_KEY_CHILD_LOCK` | `child_lock` |
| `CONFIG_GROUP_TIME` | `time` |
| `CONFIG_KEY_TZ_RULE` | `tz_rule` |
| `CONFIG_KEY_TZ_LABEL` | `tz_label` |

## Access pattern

| Phase | Behavior |
|-------|----------|
| Boot | Read all keys → populate runtime config struct (`tz_rule_init` parses POSIX `time/tz_rule` into RAM) |
| Runtime | Write on change (from MQTT `cmd/config`, provisioning, calibration, schedule update) |
| Runtime (`time/tz_rule`, `time/tz_label`) | UART `time set` and MQTT `cmd/config` both call `time_config_apply` |
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
| `.../config` | State (retained) | Full config object after any change |
| `.../cmd/config` | Command | Subset of writable settings (user-facing only) |

Not writable via MQTT: `wifi/*` (requires reprovisioning), `calib/*` (requires
calibration action), `power/batt_scale_x1000` (requires UART `adc cal`), `system/*`
(internal bookkeeping).
