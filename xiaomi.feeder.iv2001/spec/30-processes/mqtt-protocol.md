# MQTT protocol

serves:
  - ../20-stories/connectivity.md

## Topic namespace

Base: `petfeeder/<device_id>/` where `<device_id>` is user-configurable
(default: last 6 hex chars of MAC, e.g. `petfeeder/a4cf12`).

## State topics (published by device, retained)

| Topic | Payload | QoS |
|-------|---------|-----|
| `.../connection` | `online` or `offline` (plain text) | 1 |
| `.../state` | `{"bowl_error": false}` — device condition (faults / health); see [Device condition](#device-condition) | 1 |
| `.../bowl_weight` | `42` — plain integer grams; empty string when unknown; see [Bowl weight](#bowl-weight) | 1 |
| `.../hopper` | `normal` \| `low` \| `empty` (plain text) | 1 |
| `.../battery` | `75` — plain integer 0–100; `unknown` when absent; see [Battery](#battery) | 1 |
| `.../battery_voltage` | `5200` — plain integer pack mV; see [Battery pack voltage](#battery-pack-voltage) | 1 |
| `.../mains` | `ON` or `OFF` — barrel connected; see [Mains](#mains) | 1 |
| `.../schedule/list` | `[{"hour":8,"min":0,"days":127,"g":30,"enabled":true}, ...]` | 1 |
| `.../schedule/next` | `{"hour":8,"min":0,"g":30,"in_min":120}` | 1 |
| `.../config` | Config snapshot JSON — see [Config snapshot](#config-snapshot) | 1 |
| `.../timezone` | Device timezone plain text — see [Device timezone](#device-timezone) | 1 |
| `.../display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../ota/status` | `{"state": "idle", "pct": 0, "error": "", "bank": "A"}` — see [OTA status](#ota-status) | 1 |

## Event topics (published by device, not retained)

| Topic | Payload | QoS | Retain |
|-------|---------|-----|--------|
| `.../dispense/event` | Dispense completion JSON — see [Dispense event](#dispense-event) | 1 | false |

Dropped from the protocol: retained `.../dispense/status` and
`.../dispense/progress`. Completion is reported only via the fire-and-forget
dispense event. Other retained state topics (`state`, `bowl_weight`, etc.) are
unchanged.

## Command topics (subscribed by device, not retained)

| Topic | Payload | QoS |
|-------|---------|-----|
| `.../cmd/dispense` | `{"g": 30}` | 1 |
| `.../cmd/dispense/cancel` | `{}` | 1 |
| `.../cmd/schedule/set` | `{"hour":8,"min":0,"days":127,"g":30,"enabled":true}` | 1 |
| `.../cmd/schedule/delete` | `{"hour":8,"min":0}` | 1 |
| `.../cmd/calibrate` | `{"action": "zero"}` or `{"action": "span"}` | 1 |
| `.../cmd/display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../cmd/config` | `{"key": "value", ...}` | 1 |
| `.../cmd/reboot` | `{}` | 1 |
| `.../cmd/ota` | `{"url": "http://...", "sha512": "<128 hex chars>"}` — `sha512` optional | 1 |

## Last Will and Testament (LWT)

| Parameter | Value |
|-----------|-------|
| Will topic | `.../connection` |
| Will payload | `offline` |
| Will retain | true |
| Will QoS | 1 |

Set at CONNECT time. Broker publishes will on unclean disconnect. On
graceful disconnect the firmware publishes `offline` on `.../connection`
before `DISCONNECT`. `.../state` is not used for presence.

## Home Assistant discovery

On connect (and every `[tune]` 300 s when the full entity table ships),
publish discovery configs to:
`homeassistant/<component>/petfeeder_<device_id>/<object_id>/config`
(retained, QoS 1). Post-connect discovery publishes go through
[mqtt_outbox](#publish-path) — only `mqtt_io` drains the queue.

Each discovery message includes a `device` block with `identifiers`,
`name`, `manufacturer`, `model` for unified HA device grouping.
`manufacturer` and `model` identify the **physical hardware** (retail
product), not the firmware author — see [validation slice](#home-assistant-validation-slice).

### Full entity table (planned)

| Component | object_id | HA device_class | Notes |
|-----------|-----------|-----------------|-------|
| sensor | bowl_weight | `weight` | Unit: g |
| sensor | eaten_today | `weight` | Unit: g |
| sensor | battery | `battery` | Unit: % |
| sensor | hopper_level | `enum` | Options: normal, low, empty |
| binary_sensor | mains | `power` | Mains connected — `ON`/`OFF` on `.../mains` |
| button | dispense | — | Triggers default portion |
| event | dispense_completed | — | Fires on each dispense job completion |
| number | dispense_custom | — | Range: 5–150 g |
| switch | child_lock | — | On/off |
| select | display_mode | — | Options: weight, eaten_today, off |
| number | display_brightness | — | Range: 1–4 |

### Home Assistant validation slice

The firmware publishes HA entities incrementally before the full table lands.

**Dispense button** (shipped):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/button/petfeeder_<device_id>/dispense/config` |
| `command_topic` | `petfeeder/<device_id>/cmd/dispense` |
| `payload_press` | `{}` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |
| `device.identifiers` | `["petfeeder_<device_id>"]` |
| `device.manufacturer` | `Xiaomi` |
| `device.model` | `Smart Pet Food Feeder 2` |

**Bowl error** binary sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/binary_sensor/petfeeder_<device_id>/bowl_error/config` |
| `state_topic` | `petfeeder/<device_id>/state` |
| `value_template` | `{{ value_json.bowl_error }}` |
| `payload_on` / `payload_off` | JSON booleans `true` / `false` (not default `ON`/`OFF`) |
| `device_class` | `problem` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

Bench payloads: `tools/mqtt/payloads/ha-bowl_error.json`.

**Bowl weight** sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/sensor/petfeeder_<device_id>/bowl_weight/config` |
| `state_topic` | `petfeeder/<device_id>/bowl_weight` |
| `unit_of_measurement` | `g` |
| `device_class` | `weight` |
| `state_class` | `measurement` |
| `availability` | Dual — `.../connection` (`online` / `offline`) **and** `.../state` (`value_template`: `{{ value_json.bowl_error == false }}`, `payload_available`: `true`, `payload_not_available`: `false`) |

No `value_template` on the sensor — the topic payload is the gram reading directly.

Bench payloads: `tools/mqtt/payloads/ha-bowl_weight.json`, `bowl_weight-42`, `bowl_weight-empty`.

**Dispense completed** event (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/event/petfeeder_<device_id>/dispense_completed/config` |
| `name` | `Dispense completed` |
| `state_topic` | `petfeeder/<device_id>/dispense/event` |
| `event_types` | `["success", "underfill", "stuck", "empty_hopper", "aborted"]` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

Event payloads use HA MQTT event JSON on `state_topic` (non-retained). See
[Dispense event](#dispense-event). No post-connect snapshot — events are
ephemeral.

**Battery** sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/sensor/petfeeder_<device_id>/battery/config` |
| `name` | `Battery` |
| `state_topic` | `petfeeder/<device_id>/battery` |
| `unit_of_measurement` | `%` |
| `device_class` | `battery` |
| `state_class` | `measurement` |
| `availability` | Dual — `.../connection` (`online` / `offline`) **and** `.../battery` (`value_template`: `{{ 'true' if value != 'unknown' else 'false' }}`, `payload_available`: `true`, `payload_not_available`: `false`) |

No `value_template` on the sensor — payload is plain integer percentage, or `unknown` when unavailable.

**Battery pack voltage** sensor (validation slice, diagnostic):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/sensor/petfeeder_<device_id>/battery_voltage/config` |
| `name` | `Battery pack voltage` |
| `state_topic` | `petfeeder/<device_id>/battery_voltage` |
| `unit_of_measurement` | `mV` |
| `device_class` | `voltage` |
| `state_class` | `measurement` |
| `enabled_by_default` | `false` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

**Mains connected** binary sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/binary_sensor/petfeeder_<device_id>/mains/config` |
| `name` | `Mains connected` |
| `state_topic` | `petfeeder/<device_id>/mains` |
| `payload_on` | `ON` |
| `payload_off` | `OFF` |
| `device_class` | `power` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

**Hopper level** sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/sensor/petfeeder_<device_id>/hopper_level/config` |
| `name` | `Hopper level` |
| `state_topic` | `petfeeder/<device_id>/hopper` |
| `device_class` | `enum` |
| `options` | `normal`, `low`, `empty` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

No `value_template` on the sensor — payload is the level string directly.

**Device timezone** sensor (validation slice):

| Field | Value |
|-------|-------|
| Discovery topic | `homeassistant/sensor/petfeeder_<device_id>/device_timezone/config` |
| `name` | `Device timezone` |
| `state_topic` | `petfeeder/<device_id>/timezone` |
| `availability_topic` | `petfeeder/<device_id>/connection` |
| `payload_available` | `online` |
| `payload_not_available` | `offline` |

No `device_class` — payload is a free-form string (`tz_label` when set, else
POSIX `tz_rule`; always non-empty due to UTC0 fallback).

`cmd/dispense` accepts any payload; the handler ignores JSON and submits
`[tune]` 1 portion (open-loop ≈ 10 g per portion until gram-based
dispense lands). UART logs use tag `dispense` — see
[app-logging.md](app-logging.md) § Dispense diagnostics. Entity availability
follows retained `.../connection`
(`online` / `offline`).

Discovery refresh every 300 s is deferred; the outbox path supports it when
the full table ships. Stale discovery topics from prior firmware builds are
not auto-deleted on `device_id` change.

## Connection

| Parameter | Source |
|-----------|--------|
| Broker host | NVDM `mqtt/host` (from provisioning) |
| Broker port | NVDM `mqtt/port` (default 1883) |
| Username | NVDM `mqtt/user` (empty = anonymous) |
| Password | NVDM `mqtt/pass` |
| TLS | NVDM `mqtt/tls` (default false); `true` is rejected at load until a TLS adapter exists |
| Client ID | `petfeeder_<device_id>` |
| Device ID | NVDM `mqtt/device_id` when set; otherwise last 6 hex chars of STA MAC |

### Session lifecycle

Bench and runtime behavior (UART details in
[uart-console.md](uart-console.md#mqtt-commands)):

| Phase | Behavior |
|-------|----------|
| Boot | `mqtt_io` task starts; if `mqtt/host` is stored, connect when Wi-Fi STA has DHCP (no `mqtt connect` needed). Otherwise remain idle until armed |
| `mqtt connect` (or provisioning success) | Arm client; connect when Wi-Fi STA has DHCP |
| `mqtt disconnect` | Disarm for this boot; no reconnect until `mqtt connect` or reboot |
| Successful CONNECT | Subscribe `cmd/#`; publish retained `online` on `.../connection`; LWT installed; `EVT_MQTT_CONNECTED` enqueues retained `.../state`, `.../ota/status`, and HA discovery (drained by `mqtt_io`) |
| Connected idle | `mqtt_io` drains [mqtt_outbox](#publish-path) then `MQTTYield`; inbound commands are heap-copied to `app_event_q` and routed on `app` (see [Inbound commands](#inbound-commands)) |
| Session loss while armed | Exponential backoff reconnect (see below) |

### Session display

`mqtt_client_request_connect()` posts `EVT_MQTT_SESSION` (connecting) immediately.
The TCP/MQTT handshake runs on a short-lived **`mqtt_cn` worker** at priority
below `app`; `mqtt_io` (ABOVE_NORMAL) only starts the worker and polls
completion every `[tune]` 50 ms — it does not call `mqtt->connect()` inline.
Phase changes also sync at the end of each `mqtt_io` step. The `app` task maps
phase → `display_mqtt_indicator_*` — see
[app-event-loop.md](app-event-loop.md) and [display-presentation.md](display-presentation.md)
§ MQTT indicator.

While connecting, bowl-gram digits on the TM1637 continue to update at
`[tune]` 500 ms (same `app` display tick path as idle). Orange lightbar
blinks may skip occasional frames when WFCI `try_acquire` fails; weight
readings keep the last good sample on `PORT_ERR_BUSY`.

### Connect execution

| Step | Task | Behavior |
|------|------|----------|
| Arm + session sync | `mqtt_io` or `app` producer | `mqtt_client_request_connect()` sets pending flags and posts connecting phase |
| Handshake | `mqtt_cn` worker | `ConnectNetwork`, `MQTTConnect`, subscribe `cmd/#`, publish retained `connection`=`online`; `mqtt_adapter_yield` before worker exit |
| Post-connect enqueue | `app` on `EVT_MQTT_CONNECTED` | Enqueue retained `.../state`, `.../ota/status`, HA discovery — no `mqtt_port->publish` from `app` |
| Poll | `mqtt_io` | `[tune]` 50 ms step delay while worker runs; on completion apply backoff or post `EVT_MQTT_CONNECTED` |
| Disarm | `mqtt_client_stop()` | Clears worker state; disconnect if session was up |

**Yield guard:** `mqtt_io` must not call `MQTTYield` while the `mqtt_cn`
worker holds the socket.  The step function gates yield behind
`!s_connect_worker_running && mqtt->is_connected()`.  Without this guard,
select/recv on the shared socket races with the worker's subscribe call and
causes subscribe timeouts or duplicate CONNACK processing. `[design]`

Task priorities and stack sizes: [task-model.md](../40-architecture/task-model.md).

Summary:

| Session phase | Lightbar |
|---------------|----------|
| Connecting (armed, connect in flight) | Orange inverted blink |
| Connected | Green steady on |
| Backoff after failed connect (armed) | Orange error pattern |
| Disarmed, suspended, or Wi-Fi not ready | Both off |

Subscription is a single wildcard `.../cmd/#` covering the nine command topics
below — not `#`, not other devices' namespaces.

**Implemented now:** connect/LWT/`connection` presence, subscribe, reconnect backoff,
command topic classification, OTA download via `cmd/ota` (HTTP + SHA-512 verify,
A/B bank swap, slot-health confirm), [mqtt_outbox](#publish-path)
(post-connect publish queue), device condition (`bowl_error` on `.../state`),
`bank` on every `ota/status`, HA validation-slice **Dispense** button, **Bowl
error** binary_sensor, **Bowl weight** sensor, **Battery** sensor, **Mains
connected** binary_sensor, and **Dispense completed** event discovery,
`cmd/dispense` → one portion, `.../dispense/event` on job completion,
`.../battery`, `.../mains`, `.../hopper`, and `.../timezone` telemetry publishers,
HA validation-slice **Hopper level** and **Device timezone** sensors.

**Partially implemented:** `.../bowl_weight` telemetry publisher (validation slice);
`.../config` time/TZ slice (`tz_rule`, `tz_label`, `time_synced`, `utc_epoch`);
`cmd/config` for `tz_rule` and `tz_label`.

**Not implemented yet:** remaining telemetry topics (`eaten_today`,
schedule, display), additional HA entities from the full table,
300 s discovery refresh, non-dispense command handlers, per-topic last-value-wins
coalescing on the outbox.

### Publish path

Post-connect MQTT publishes use a ring-buffered **mqtt_outbox** owned by
`mqtt_io`:

| Phase | Who may call `mqtt_port->publish()` |
|-------|-------------------------------------|
| `mqtt_cn` handshake | Connect worker only (retained `connection`=`online`; subscribe; LWT set at CONNECT) |
| Connected session | **`mqtt_io` only** — `mqtt_adapter_yield` before each outbox drain; one item per step |
| `app` task | **Enqueue only** — copy topic and payload into the outbox |

| Parameter | Value |
|-----------|-------|
| Ring depth | `[tune]` 16 slots |
| Max payload per slot | `[tune]` 768 B (HA discovery with dual availability) |
| Min interval between successful drains | `[tune]` 100 ms |

When the ring is full, new enqueues are dropped and a debug log is emitted.
While MQTT is suspended for OTA (`mqtt_client_suspend_for_ota`), the outbox
stops accepting enqueues — callers get a silent drop with pending count 0.
`mqtt_outbox` is cleared on disconnect. OTA status, HA discovery, and future
state publishers share this path.

### Inbound commands

Broker → device commands do not use a separate inbox module. `mqtt_io`
receives in the MQTT yield callback, heap-copies topic and payload, posts
`EVT_MQTT_MESSAGE`, and returns. `app` runs `mqtt_route_classify` and
dispatches handlers — never inside the MQTT callback or `mqtt_io` step.

### Reconnect strategy

Exponential backoff on disconnect:

- Initial delay: `[tune]` 1 s.
- Multiply by 2 on each failure.
- Cap at `[tune]` 60 s.
- Reset to initial delay on successful connect.

## Rate limiting

| Context | Max publish rate | Source |
|---------|-----------------|--------|
| Outbox drain (all post-connect TX) | 1 publish per `[tune]` 100 ms | `[design]` |
| State topics (general, when implemented) | 1 per topic per `[tune]` 2 s | `[design]` |
| Dispense event | 1 per completed job; drop when MQTT offline | `[design]` |

The outbox drain interval spaces connect-time bursts (e.g. ten HA entities
≈ 1 s). Per-topic last-value-wins coalescing for high-rate state publishers
is deferred until those topics ship.

## Config snapshot

Topic `.../config` (retained, QoS 1). Time/TZ fields in this slice; other
keys ship with future `cmd/config` handlers.

| Field | Type | Semantics |
|-------|------|-----------|
| `tz_rule` | string | POSIX TZ from [scheduler-engine.md](scheduler-engine.md); default `"UTC0"` |
| `tz_label` | string | Display-only IANA name; `""` when unset; max 47 UTF-8 bytes |
| `time_synced` | bool | `true` after first successful NTP this boot |
| `utc_epoch` | int | Unix seconds from RTC when synced; `0` when unknown |

Example:

```json
{"tz_rule":"EET-2EEST,M3.5.0/3,M10.5.0/4","tz_label":"Europe/Bucharest","time_synced":true,"utc_epoch":1718841600}
```

Publish on boot (after NVDM load), first NTP success, periodic NTP re-sync,
`cmd/config` change, and MQTT connect snapshot.

### `cmd/config` (time slice)

Writable keys: `tz_rule`, `tz_label`. Unknown keys are rejected. See
[config-store.md](config-store.md).

| Key | Empty string |
|-----|--------------|
| `tz_label` | Clears the label (erases NVDM key; display falls back to `tz_rule`) |
| `tz_rule` | Resets to default UTC0 (erases NVDM key; scheduler uses UTC) |

Publishes updated retained `.../config` and `.../timezone` on success.

## Device timezone

Topic `.../timezone` (retained, QoS 1) reports the effective timezone string
for display and Home Assistant.

| Payload | When |
|---------|------|
| `tz_label` value | NVDM `time/tz_label` is set and non-empty |
| POSIX `tz_rule` | Label unset; e.g. `UTC0`, `EET-2EEST,M3.5.0/3,M10.5.0/4` |

The payload is always non-empty: missing or cleared `tz_rule` loads as
`UTC0` (see [config-store.md](config-store.md)).

Publish on boot (MQTT connect snapshot), `cmd/config` change, and UART
`time set` when MQTT is configured.

## Device condition

Topic `.../state` (retained, QoS 1) reports operational health — not presence
(`.../connection`) and not OTA slot metadata (`ota/status.bank`).

| Field | Type | When `true` |
|-------|------|-------------|
| `bowl_error` | bool | Weigh cal incomplete, span pending, or calibrated bowl missing (see [weighing.md](weighing.md) § Bowl presence) |

Publish on edge change and as a snapshot after MQTT connect. Not on every
weight sample tick.

Example payloads:

```json
{"bowl_error": false}
{"bowl_error": true}
```

Bench templates: `tools/mqtt/payloads/state-ok.json`,
`state-bowl-error.json`.

## Bowl weight

Topic `.../bowl_weight` (retained, QoS 1) reports food grams in the bowl now.
Plain integer string — not JSON. Separate from `.../state` (health) and from
future `.../eaten_today` (cumulative consumption counter).

| Payload | Meaning |
|---------|---------|
| `42` | Calibrated, bowl present, valid sample — 42 g food |
| `0` | Empty bowl or small negative drift clamped to zero |
| `""` (empty) | Unknown — uncalibrated, bowl missing, no valid sample, or implausible reading |

Presentation rules match panel **weight** mode ([display-presentation.md](display-presentation.md)
§ Display modes) except MQTT publishes the **full** calibrated range (no 999 g
display cap). See [weighing.md](weighing.md) § Bowl presence and data model.

Publish triggers (not on every `[tune]` 500 ms weight sample):

| Trigger | Publish |
|---------|---------|
| MQTT connect | Retained snapshot (`N` or `""`) |
| Post-dispense resample | Force snapshot |
| Known grams change by ≥ `[tune]` 2 g | Yes |
| Known ↔ unknown transition | Yes |
| Stable within 2 g deadband | No |

Rate cap: at most **1 publish per `[tune]` 2 s** on this topic; coalesce rapid
changes to the latest value within the window. Post-dispense force bypasses the
coalesce delay.

Home Assistant marks the entity unavailable when `bowl_error` is `true` on
`.../state` (dual `availability` in discovery) so a stale retained gram value is
not shown as a live reading while the bowl is missing or cal is incomplete.

## Battery

Topic `.../battery` (retained, QoS 1) reports pack state of charge as a plain
integer percentage 0–100. Separate from `.../mains` (power source) and
`.../battery_voltage` (diagnostic raw mV).

| Payload | Meaning |
|---------|---------|
| `75` | 75 % remaining (from ADC + discharge curve) |
| `0` | Depleted but present pack (~≤ 4.0 V) |
| `unknown` | Unknown — pack ADC reading exactly 0 mV (no cells / open circuit) |

Use the literal `unknown` (not an empty payload): Mosquitto and most brokers
delete retained messages on zero-length publish, so HA would keep showing the
last numeric state.

Home Assistant marks the entity unavailable when the payload is `unknown`
(dual `availability` in discovery). The battery-topic availability entry must
use a template that renders the strings `true` or `false` — for example
`{{ 'true' if value != 'unknown' else 'false' }}` — because HA compares the
template result to `payload_available` / `payload_not_available` as strings.
A bare boolean expression such as `{{ value != 'unknown' }}` renders `True` or
`False` and does not match `"false"`, leaving the entity available with a
stale percentage.

Samples before MQTT topics are configured (timer tick before first connect) are
ignored so a pre-connect reading cannot be snapshotted as the connect retain.
On connect, ADC is force-sampled before the battery connect snapshot.

Publish triggers:

| Trigger | Publish |
|---------|---------|
| MQTT connect | Retained snapshot of last known state |
| Mains/battery transition | Force resample + publish |
| Known ↔ unknown transition | Yes |
| Percentage change ≥ `[tune]` 1 pt (when known) | Yes |
| Periodic ADC sample (no change) | No |

Sample interval from debounced power source ([battery-monitoring.md](battery-monitoring.md)):
`[tune]` 60 s on battery, `[tune]` 300 s on mains. ADC read skipped on
`PORT_ERR_BUSY` (motor running / WFCI contended).

When `read_battery_mv()` returns exactly **0 mV**, publish `unknown` on this
topic (not `"0"`). Values `> 0 mV` map through the discharge curve including 0 %.

Chemistry enum and knot tables: [battery-monitoring.md](battery-monitoring.md)
§ Chemistry and discharge curves.

## Battery pack voltage

Topic `.../battery_voltage` (retained, QoS 1) reports raw scaled pack voltage
in millivolts. Diagnostic / integrator topic — always publishes the ADC result
(including `0`).

| Payload | Meaning |
|---------|---------|
| `5200` | 5.2 V pack |

Publish triggers (same sample tick as [Battery](#battery)):

| Trigger | Publish |
|---------|---------|
| MQTT connect | Retained snapshot |
| Mains/battery transition | Force resample + publish |
| Change ≥ `[tune]` 10 mV | Yes |
| Periodic ADC sample (no change) | No |

Home Assistant discovers **Battery pack voltage** with `enabled_by_default`:
`false` — hidden until enabled in the UI.

## Hopper

Topic `.../hopper` (retained, QoS 1) reports the published three-state hopper
level (see [hopper-sensing.md](hopper-sensing.md) `hopper_level`).

| Payload | Meaning |
|---------|---------|
| `normal` | IR beam blocked or hopper not latched empty |
| `low` | IR almost empty; hopper not latched empty |
| `empty` | Confirmed out-of-food or compensated underfill |

Publish on published-level edge change and as a connect snapshot after MQTT
connect (`mqtt_hopper_connect_snapshot`). Not tied to the IR background poll
interval.

## Mains

Topic `.../mains` (retained, QoS 1) reports whether the barrel adapter is
present (debounced P1.1 sense).

| Payload | Meaning |
|---------|---------|
| `ON` | Running on mains (barrel connected) |
| `OFF` | Running on battery |

Publish on debounced edge change and as a snapshot after MQTT connect. Not
tied to battery sample interval.

## Dispense event

Topic `.../dispense/event` (QoS 1, **not retained**). Published once per
terminal dispense job when MQTT is connected at completion. If MQTT is offline
when the job finishes, the event is **dropped** — no replay buffer.

Home Assistant discovers an **event** entity (`object_id`: `dispense_completed`,
`name`: `Dispense completed`). The UI shows the outcome above the fold as the
event type (e.g. **Dispense completed** → `stuck`). Automations trigger on
`event_type` (`success`, `stuck`, etc.) and read properties such as `grams`.

### Payload fields

| Field | Type | When present |
|-------|------|--------------|
| `event_type` | string | Always — terminal outcome: `success`, `underfill`, `stuck`, `empty_hopper`, `aborted` |
| `grams` | int | Always — clamped ≥ 0 in payload |
| `grams_estimated` | bool | Always |
| `target_g` | int | Always — portion mode: `portions × 10`; gram mode: request target |
| `source` | string | Always — `mqtt`, `uart`, `button`, `schedule` (when implemented) |
| `mode` | string | Always — `open_loop` or `compensated` |
| `batch_count` | int | Always — motor batches in job (1 in open-loop portion v1) |
| `deficit_g` | int | Compensated mode only — `max(0, target_g − grams)` |

When `grams_estimated` is `true`, `grams` is a motor-based fallback
(`portions × 10`) because the scale read failed or `bowl_error` was active.
When `false`, `grams` is the measured bowl delta (`post_settle − baseline`).

Example (measured success):

```json
{
  "event_type": "success",
  "grams": 28,
  "grams_estimated": false,
  "target_g": 30,
  "source": "mqtt",
  "mode": "open_loop",
  "batch_count": 1
}
```

Example (estimated — bowl error):

```json
{
  "event_type": "success",
  "grams": 10,
  "grams_estimated": true,
  "target_g": 10,
  "source": "button",
  "mode": "open_loop",
  "batch_count": 1
}
```

Example (stuck):

```json
{
  "event_type": "stuck",
  "grams": 5,
  "grams_estimated": false,
  "target_g": 10,
  "source": "uart",
  "mode": "open_loop",
  "batch_count": 1
}
```

Negative raw bowl deltas are clamped to **0** in `grams`; the signed raw delta
feeds internal hopper-empty detection (see [dispense-cycle.md](dispense-cycle.md)).

## OTA status

Topic `.../ota/status` (retained, QoS 1). Published on OTA progress and after
MQTT connect (`state` reset to `idle`). Post-connect status updates are
enqueued on `app` and drained by `mqtt_io` — not published directly from
the OTA handler.

| Field | Values |
|-------|--------|
| `state` | `idle`, `downloading`, `applying`, `error` |
| `pct` | 0–100 download progress |
| `error` | Empty string on success paths; when `state` is `error`, one of: |
| `bank` | Active application partition: `"A"` or `"B"` — included on every publish |

| `error` value | When |
|---------------|------|
| `invalid_url` | Malformed or oversized `cmd/ota` payload, bad URL scheme, invalid `sha512` hex, or `start` rejected |
| `already_in_progress` | OTA already running (`PORT_ERR_BUSY`) |
| `download_failed` | HTTP download or flash write failure |
| `verify_failed` | SHA-512 mismatch or inactive-bank verify failure |
| `image_too_large` | Image exceeds inactive bank capacity |

### `cmd/ota` payload

| Field | Required | Description |
|-------|----------|-------------|
| `url` | yes | HTTP or HTTPS firmware URL (max 255 chars) |
| `sha512` | no | Expected image SHA-512 as 128 lowercase/uppercase hex digits; when omitted, verify uses the hash computed from the downloaded image |
