# Connectivity

## Overview

The feeder communicates over MQTT on the local network. There is no
cloud dependency and no phone-home telemetry.

## Design principles

- **Home Assistant auto-discovery** — entities appear automatically.
- **Generic MQTT compatible** — works with any broker and automation
  platform (Homey, Node-RED, etc.).
- **Neutral namespace** — topic paths use `petfeeder/<device_id>/...`
  with no vendor-specific prefixes.
- **JSON payloads** — simple, flat JSON; no binary encoding.
- **Retained state** — state topics are retained so new subscribers get
  the latest values immediately.

## Online / offline presence

- The device uses an MQTT Last Will and Testament (LWT) to report
  online/offline status. If the device disconnects unexpectedly the
  broker publishes the offline message automatically.

## State topics (device → user)

The feeder publishes the following retained state topics:

- **Weight** — bowl grams, eaten-today grams.
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
- **Calibrate** — tare or span the weight sensor.
- **Display** — set mode and brightness.
- **Config** — update user-facing settings.
- **Reboot** — restart the device.

## Connection

- Broker address, port, and credentials are set during provisioning.
- TLS is optional (configurable).
- Automatic reconnect with exponential backoff on disconnect.
