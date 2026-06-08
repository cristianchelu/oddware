# OTA bench tools

Scripts to trigger and validate A/B firmware updates over MQTT from a
development machine. The feeder downloads the inactive-bank image from a
local HTTP server with Range support; see
[`spec/30-processes/ota-flow.md`](../../spec/30-processes/ota-flow.md) for
device behaviour.

## Prerequisites

- Device online on the same LAN as this host
- MQTT broker reachable (`mosquitto_pub`, `mosquitto_sub`)
- `python3`, `curl`, `fuser` (for the Range HTTP server and port cleanup)
- Built images in `firmware/flash/` (`petfeeder_a.bin`, `petfeeder_b.bin`)
- Optional: USB-serial on `/dev/ttyUSB0` for boot/OTA UART capture

## Configuration

```bash
cp tools/ota/.env.example tools/ota/.env   # once per machine
```

Edit `.env` with your broker address and credentials, or export variables
before running. Existing env values are not overwritten.

| Variable | Default | Purpose |
|----------|---------|---------|
| `MQTT_HOST` | `127.0.0.1` | Broker address |
| `MQTT_PORT` | `1883` | Broker port |
| `MQTT_USER` / `MQTT_PASS` | see file | Broker credentials |
| `HTTP_PORT` | `8080` | Range HTTP server |
| `UART_DEV` | `/dev/ttyUSB0` | Serial port for capture |
| `HOP_TIMEOUT` | `300` | Max seconds per hop |
| `PROGRESS_TIMEOUT` | `90` | Stall limit with no download progress |

Bench logs land in `tools/ota/logs/<run-id>/` (gitignored). Set
`OTA_LOG_KEEP=0` to delete successful runs on exit.

## Commands

```bash
# Single hop: wait for MQTT ready, publish inactive bank, wait for swap
./tools/ota/mqtt-ota.sh --device-id 768722 --skip-build

# Publish only (HTTP server stays up in background)
./tools/ota/mqtt-ota.sh --device-id 768722 --publish-only --skip-build

# A → B → A round-trip validation (two hops, shared HTTP + UART lock)
./tools/ota/ota-ab-roundtrip.sh 768722
```

Lower-level scripts (`ota-hop.sh`, `ota-wait-hop.sh`, `wait-mqtt-online.sh`)
are used by the wrappers; run `ota-hop.sh --help` for options.
