# MQTT protocol

serves:
  - ../20-stories/connectivity.md

## Topic namespace

Base: `petfeeder/<device_id>/` where `<device_id>` is user-configurable
(default: last 6 hex chars of MAC, e.g. `petfeeder/a4cf12`).

## State topics (published by device, retained)

| Topic | Payload | QoS |
|-------|---------|-----|
| `.../state` | `{"online": true, "bank": "A"}` or `{"online": true, "bank": "B"}` | 1 |
| `.../weight` | `{"bowl_g": 42, "eaten_today_g": 85}` | 1 |
| `.../hopper` | `{"level": "normal"}` | 1 |
| `.../power` | `{"source": "mains", "battery_pct": 100}` | 1 |
| `.../dispense/status` | `{"state": "idle", "last_result": "success", "last_g": 30}` | 1 |
| `.../dispense/progress` | `{"pct": 65, "target_g": 30}` | 1 |
| `.../schedule/list` | `[{"hour":8,"min":0,"days":127,"g":30,"enabled":true}, ...]` | 1 |
| `.../schedule/next` | `{"hour":8,"min":0,"g":30,"in_min":120}` | 1 |
| `.../config` | `{...full config object...}` | 1 |
| `.../display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../ota/status` | `{"state": "idle", "pct": 0, "error": ""}` — see [OTA status](#ota-status) | 1 |

## Command topics (subscribed by device, not retained)

| Topic | Payload | QoS |
|-------|---------|-----|
| `.../cmd/dispense` | `{"g": 30}` | 1 |
| `.../cmd/dispense/cancel` | `{}` | 1 |
| `.../cmd/schedule/set` | `{"hour":8,"min":0,"days":127,"g":30,"enabled":true}` | 1 |
| `.../cmd/schedule/delete` | `{"hour":8,"min":0}` | 1 |
| `.../cmd/calibrate` | `{"action": "tare"}` or `{"action": "span", "g": 200}` | 1 |
| `.../cmd/display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../cmd/config` | `{"key": "value", ...}` | 1 |
| `.../cmd/reboot` | `{}` | 1 |
| `.../cmd/ota` | `{"url": "http://...", "sha512": "<128 hex chars>"}` — `sha512` optional | 1 |

## Last Will and Testament (LWT)

| Parameter | Value |
|-----------|-------|
| Will topic | `.../state` |
| Will payload | `{"online": false}` |
| Will retain | true |
| Will QoS | 1 |

Set at CONNECT time. Broker publishes will on unclean disconnect.

## Home Assistant discovery

On connect (and every `[tune]` 300 s), publish discovery configs to:
`homeassistant/<component>/petfeeder_<device_id>/<entity>/config` (retained, QoS 1).

| Component | Entity ID | HA device_class | Notes |
|-----------|-----------|-----------------|-------|
| sensor | bowl_weight | `weight` | Unit: g |
| sensor | eaten_today | `weight` | Unit: g |
| sensor | battery | `battery` | Unit: % |
| sensor | hopper_level | `enum` | Options: normal, low |
| binary_sensor | power_mains | `power` | On/off |
| button | dispense_10g | — | Triggers default portion |
| number | dispense_custom | — | Range: 5–150 g |
| switch | child_lock | — | On/off |
| select | display_mode | — | Options: weight, eaten_today, off |
| number | display_brightness | — | Range: 1–4 |

Each discovery message includes a `device` block with `identifiers`,
`name`, `manufacturer`, `model` for unified HA device grouping.

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
| Successful CONNECT | Subscribe `petfeeder/<device_id>/cmd/#`; publish retained `{"online": true, "bank": "A"|"B"}` on `.../state` (`bank` is the active A/B partition); install LWT per table above |
| Connected idle | Process inbound commands via `MQTTYield`; route known `cmd/*` topics silently (handlers `[design]`) |
| Session loss while armed | Exponential backoff reconnect (see below) |

### Session display

Status lightbar updates from `mqtt_client.c` via
`display_mqtt_indicator.c` — see [display-presentation.md](display-presentation.md)
§ MQTT indicator. Summary:

| Session phase | Lightbar |
|---------------|----------|
| Connecting (armed, connect in flight) | Orange inverted blink |
| Connected | Green steady on |
| Backoff after failed connect (armed) | Orange error pattern |
| Disarmed, suspended, or Wi-Fi not ready | Both off |

Subscription is a single wildcard `.../cmd/#` covering the nine command topics
below — not `#`, not other devices' namespaces.

**Implemented now:** connect/LWT/online state, subscribe, reconnect backoff,
command topic classification, OTA download via `cmd/ota` (HTTP + SHA-512 verify,
A/B bank swap, post-boot rollback timer).

**Not implemented yet:** Home Assistant discovery, other retained state topics,
non-OTA command handlers, rate limiting.

### Reconnect strategy

Exponential backoff on disconnect:

- Initial delay: `[tune]` 1 s.
- Multiply by 2 on each failure.
- Cap at `[tune]` 60 s.
- Reset to initial delay on successful connect.

## Rate limiting

| Context | Max publish rate | Source |
|---------|-----------------|--------|
| State topics (general) | 1 per topic per `[tune]` 2 s | `[design]` |
| Dispense progress | 1 per `[tune]` 500 ms | `[design]` |

Newer values supersede queued publishes (last-value-wins, not queued).

## OTA status

Topic `.../ota/status` (retained, QoS 1). Published on OTA progress and after
MQTT connect (`state` reset to `idle`).

| Field | Values |
|-------|--------|
| `state` | `idle`, `downloading`, `applying`, `error` |
| `pct` | 0–100 download progress |
| `error` | Empty string on success paths; when `state` is `error`, one of: |

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
