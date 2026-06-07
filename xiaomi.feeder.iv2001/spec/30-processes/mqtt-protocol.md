# MQTT protocol

serves:
  - ../20-stories/connectivity.md

## Topic namespace

Base: `petfeeder/<device_id>/` where `<device_id>` is user-configurable
(default: last 6 hex chars of MAC, e.g. `petfeeder/a4cf12`).

## State topics (published by device, retained)

| Topic | Payload | QoS |
|-------|---------|-----|
| `.../state` | `{"online": true}` | 1 |
| `.../weight` | `{"bowl_g": 42, "eaten_today_g": 85}` | 1 |
| `.../hopper` | `{"level": "normal"}` | 1 |
| `.../power` | `{"source": "mains", "battery_pct": 100}` | 1 |
| `.../dispense/status` | `{"state": "idle", "last_result": "success", "last_g": 30}` | 1 |
| `.../dispense/progress` | `{"pct": 65, "target_g": 30}` | 1 |
| `.../schedule/list` | `[{"hour":8,"min":0,"days":127,"g":30,"enabled":true}, ...]` | 1 |
| `.../schedule/next` | `{"hour":8,"min":0,"g":30,"in_min":120}` | 1 |
| `.../config` | `{...full config object...}` | 1 |
| `.../display` | `{"mode": "weight", "brightness": 4}` | 1 |
| `.../ota/status` | `{"state": "idle", "pct": 0, "error": ""}` | 1 |

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
| `.../cmd/ota` | `{"url": "http://..."}` | 1 |

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
| TLS | NVDM `mqtt/tls` (default false); depends on available RAM for mbedTLS |
| Client ID | `petfeeder_<device_id>` |

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
