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
| `.../hopper` | `{"level": "normal"}` | 1 |
| `.../power` | `{"source": "mains", "battery_pct": 100}` | 1 |
| `.../dispense/status` | `{"state": "idle", "last_result": "success", "last_g": 30}` | 1 |
| `.../dispense/progress` | `{"pct": 65, "target_g": 30}` | 1 |
| `.../schedule/list` | `[{"hour":8,"min":0,"days":127,"g":30,"enabled":true}, ...]` | 1 |
| `.../schedule/next` | `{"hour":8,"min":0,"g":30,"in_min":120}` | 1 |
| `.../config` | `{...full config object...}` | 1 |
| `.../display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../ota/status` | `{"state": "idle", "pct": 0, "error": "", "bank": "A"}` — see [OTA status](#ota-status) | 1 |

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
| sensor | hopper_level | `enum` | Options: normal, low |
| binary_sensor | power_mains | `power` | On/off |
| button | dispense | — | Triggers default portion |
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
error** binary_sensor, and **Bowl weight** sensor discovery, `cmd/dispense` → one portion.

**Partially implemented:** `.../bowl_weight` telemetry publisher (validation slice).

**Not implemented yet:** remaining telemetry topics (`hopper`, `power`, `eaten_today`,
dispense, schedule, config, display), additional HA entities from the full table,
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
| Dispense progress (when implemented) | 1 per `[tune]` 500 ms | `[design]` |

The outbox drain interval spaces connect-time bursts (e.g. ten HA entities
≈ 1 s). Per-topic last-value-wins coalescing for high-rate state publishers
is deferred until those topics ship.

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
