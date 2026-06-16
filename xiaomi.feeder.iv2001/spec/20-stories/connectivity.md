# Connectivity

## Overview

The feeder communicates over MQTT on the local network. There is no
cloud dependency and no phone-home telemetry.

## Design principles

- **Home Assistant auto-discovery** — entities appear automatically. Shipped
  today: **Dispense** button and **Bowl error** binary sensor; the full entity
  table is in [mqtt-protocol.md](../30-processes/mqtt-protocol.md) § Full entity
  table (planned).
- **Generic MQTT compatible** — works with any broker and automation
  platform (Homey, Node-RED, etc.).
- **Neutral namespace** — topic paths use `petfeeder/<device_id>/...`
  with no vendor-specific prefixes.
- **JSON payloads** — simple, flat JSON; no binary encoding.
- **Retained state** — state topics are retained so new subscribers get
  the latest values immediately.

## Online / offline presence

- Presence is on `petfeeder/<device_id>/connection` — plain-text `online` or
  `offline` (retained, QoS 1). Device condition and OTA metadata use other
  topics (`.../state`, `.../ota/status`).
- The device uses an MQTT Last Will and Testament (LWT) on `.../connection`.
  If the device disconnects unexpectedly the broker publishes `offline`
  automatically.

## State topics (device → user)

The feeder publishes the following retained state topics:

- **Weight** — bowl grams, eaten-today grams.
- **Device condition** — faults and health (`bowl_error`, extensible).
- **Hopper** — fill level (normal / low).
- **Power** — source (mains / battery), battery percentage.
- **Dispense status** — idle or active, last result, last grams.
- **Dispense progress** — live percentage during a dispense.
- **Schedule list** — all configured slots.
- **Next scheduled feed** — time and gram amount.
- **Config** — current device settings.
- **Display** — current mode and brightness.

## Command topics (user → device)

The feeder subscribes to commands for:

- **Dispense** — trigger with a gram amount, or cancel.
- **Schedule CRUD** — create, update, delete schedule slots.
- **Calibrate** — zero or span the weight sensor.
- **Display** — set mode and brightness.
- **Config** — update user-facing settings.
- **Reboot** — restart the device.

## Connection

- Broker address, port, and credentials are set during provisioning.
- TLS is optional (configurable).
- Automatic reconnect with exponential backoff on disconnect.

## On-panel broker status

The feeder shows MQTT session state on the status lightbar (see
[display.md](display.md) § MQTT / broker indicator): green when the broker
session is up, orange patterns while connecting or after a failed connect
during reconnect backoff. Bowl-gram digits keep updating on the default
**weight** display mode during the connecting phase (see
[app-event-loop.md](../30-processes/app-event-loop.md) § Coexistence with MQTT
connect).
