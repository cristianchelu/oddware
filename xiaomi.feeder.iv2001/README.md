# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2**
(retail XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker, plus an
optional LAN web admin UI. No cloud, no MIoT, no phone-home.

## Early, but usable with Home Assistant

This firmware is **experimental and early**. APIs, defaults, and behavior can
change between releases; some product features from the stock feeder are still
missing (pin-hole reset, sleep mode, TLS, and others in the checklist below).

For a **Home Assistant** setup on a local MQTT broker, it is already **fully
usable for normal day-to-day feeding**: edit schedules from HA or the LAN web
UI, dispense on demand (button or automation), and rely on scheduled feeds at
the configured times. Each completed dispense publishes a **Dispense completed**
event to MQTT with measured grams and outcome (`success`, `underfill`, `stuck`,
etc.) so you can build automations — notifications, logging, feeding history,
or integrations with other pets/devices — on the HA side without cloud services.

## Firmware status

Active development. The checklist below tracks breadth of coverage; user goals
live in [`spec/20-stories/`](spec/20-stories/), detailed behavior in
[`spec/30-processes/`](spec/30-processes/).

### Build & recovery

- [x] IV2001 board build (MT7682, A/B partitions, custom bootloader)
- [x] Host unit tests (`make test-host`)
- [x] Web UI client tests (`make test-web`)
- [x] UART flash tooling (Linux)
- [x] MQTT OTA bench loop ([`tools/ota/`](tools/ota/))
- [x] UART development console (MiniCLI)
- [x] Remote telnet console (bench; optional `REMOTE_CLI_ENABLE`)

### Connectivity

- [x] Wi-Fi STA (stored credentials, connect lifecycle)
- [x] MQTT broker session (LWT on `connection`, device condition on `state`, reconnect backoff)
- [x] MQTT OTA command (`cmd/ota`)
- [ ] TLS
- [x] Home Assistant / MQTT integration [partial] — auto-discovery for dispense button,
  bowl error, bowl weight, battery, battery pack voltage (diagnostic), mains connected,
  hopper level, device timezone, feeding schedule (with full JSON attributes),
  dispense completed event, weight compensation, overfill protection, and overfill
  threshold; pending: child lock, custom dispense grams, eaten today, display mode,
  display brightness (see [`mqtt-protocol.md`](spec/30-processes/mqtt-protocol.md))
- [x] LAN web admin UI on port 80 when STA is up (`WEB_UI_ENABLE=y`; see
  [`web-ui.md`](spec/30-processes/web-ui.md))

### Provisioning

- [x] Captive portal on first boot (home Wi-Fi + MQTT via `PetFeeder-XXXX` AP)
- [x] Factory reset via UART CLI (`factory-reset`)
- [ ] Re-provisioning via pin-hole reset (short press)
- [ ] Factory reset via pin-hole reset (long press)

### Feeding

- [x] Open-loop portion dispense (button, UART, MQTT, web UI, HA button)
- [x] Gram-targeted dispense (5–150 g via MQTT `{"g":…}`, schedule slots, UART
  `dispense grams`, web UI)
- [x] Compensated dispense mode (weight-based top-up; MQTT, HA switch, web UI, UART)
- [x] Dispense completion events over MQTT / HA (`dispense/event` with grams and outcome)
- [x] Motor index tracking and jam / anti-jam
- [x] Overfill protection — skip scheduled feeds when the bowl is already full
  (MQTT, HA, web UI, UART; manual dispense always bypasses)
- [ ] Dispense live progress (retained status topic; not in protocol)

### Sensing & display

- [x] Bowl weight on panel (default display mode) and MQTT (`bowl_weight`)
- [x] Weight scale calibration (CLI: `weigh cal zero` / `span`)
- [x] Hopper level sensing (IR + dispense weight check; UART and MQTT `hopper`)
- [x] Status pictographs (Wi-Fi, MQTT, dispense, bowl error, child lock)
- [x] Bowl missing / calibration fault detection (`bowl_error` on `state`, HA sensor)
- [ ] Alternate display modes (eaten today, off) from product paths
- [ ] MQTT `cmd/calibrate` and `cmd/display` (UART/CLI paths exist)

### Controls

- [x] Physical button input and gesture detection
- [x] Manual dispense button (short press)
- [x] Child lock [partial] — reset+dispense combo, persistent NVRAM, display feedback;
  MQTT / HA entity pending (`cmd/config` does not accept `child_lock` yet)
- [ ] Rear power button sleep / wake
- [ ] Pin-hole reset gestures (see Provisioning)

### Power & battery

Goals are split across [`monitoring.md`](spec/20-stories/monitoring.md) (battery
reporting, conservation) and [`controls.md`](spec/20-stories/controls.md)
(sleep); mechanism in
[`power-state-machine.md`](spec/30-processes/power-state-machine.md) and
[`battery-monitoring.md`](spec/30-processes/battery-monitoring.md).

- [x] Mains vs battery source detection (UART and MQTT `mains`)
- [x] Battery voltage and percentage (MQTT `battery`, `battery_voltage`; HA sensors)
- [ ] Power state machine (Normal / Battery / Sleep)
- [ ] Low-battery warning and conservation (display / Wi-Fi off; keep dispense)
- [ ] Wi-Fi on battery (`on` / `off` / `scheduled_only`)
- [ ] Sleep / wake (rear power button)

### Scheduling

- [x] NTP time sync over Wi-Fi (POSIX `TZ` in config; local civil times)
- [x] Schedule slots — up to 32 entries (MQTT CRUD, LAN web UI, UART `schedule`)
- [x] Global enable, today-only override, per-slot skip, and runtime status
  (`pending`, `skipped`, `skipped_full`, `dispensed`, …)
- [x] Next-feed reporting (`schedule/next` MQTT topic and web API)
- [ ] Eaten-today tracking and MQTT sensor

## Quick start

From this directory:

```bash
./tools/bootstrap.sh
```

Fetches the Airoha IoT SDK into gitignored `external/`, runs host unit tests, and
sources `build-env.sh`.

```bash
./tools/bootstrap.sh --with-flash-tool   # also extracts MediaTek IoT Flash Tool
./tools/bootstrap.sh --host-only         # host tests only, no SDK fetch
make test-host                           # re-run host tests
make test-web                            # web UI logic tests
make preview-web                         # local admin UI with mock API
```

### Prerequisites

| Tool | Needed for | Fedora | Debian/Ubuntu |
|------|------------|--------|---------------|
| `git`, `make`, `gcc` | Host tests | `dnf install git make gcc` | `apt install git make gcc` |
| `node` (`npx`) | Web UI client tests (`make test-web`); admin UI minify at firmware build (`WEB_UI_ENABLE=y`) | `dnf install nodejs` | `apt install nodejs` |
| `patch` | SDK patches | `dnf install patch` | `apt install patch` |
| `arm-none-eabi-gcc` + newlib | Board firmware | `dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib` | `apt install gcc-arm-none-eabi` |
| `wine`, `unzip` | Linux UART flash | `dnf install wine unzip` | `apt install wine unzip` |

First firmware build with the admin UI enabled may download `html-minifier-terser`
via `npx` (network once; cached afterward). See `tools/web/README.md`.

### Firmware build

```bash
source tools/build-env.sh
./tools/build-firmware.sh
```

See `firmware/README.md` for the GCC scaffold layout.

When `WEB_UI_ENABLE=y` (default in `feature.mk`), the build embeds a minified
gzip bundle of `tools/web/`. After flashing, open `http://<feeder-ip>/` on the
LAN for schedule editing, manual dispense, feed mode, overfill settings, and a
live status panel — no MQTT broker required.

### Flashing (Linux, UART)

Harness: 3.3 V USB-TTL, **TX→TP2**, **RX→TP1**, **GND→TP27**. Details in
`spec/10-hardware/flash.md`.

```bash
./tools/setup-flashtool.sh              # once, after bootstrap
source tools/build-env.sh && ./tools/build-firmware.sh
./tools/iot-flash.sh download /dev/ttyUSB0
```

The script starts CODA under Wine, maps the tty to `COM3`, then waits for a
manual reset (power-cycle or pulse TP15) within `IOT_FLASH_RESET_WAIT_SEC`
(default 30, in `tools/iot-flash.env`). Stable paths such as
`/dev/serial/by-id/usb-...` work.

| Command | Purpose |
|---------|---------|
| `iot-flash.sh download DEVICE` | Headless flash (CODA CLI) |
| `iot-flash.sh probe DEVICE` | BROM sync test (`0xA0` / `0x5F`) |
| `iot-flash.sh gui DEVICE` | Qt Flash Tool GUI |
| `uart-console.sh DEVICE` | Boot log (`UART_CONSOLE_SECS=N`) or picocom |

Headless full-chip readback via CODA is not supported under Wine; use the
Windows IoT Flash Tool. Reference INI: `firmware/flash/coda_readback_2mb.ini`.

Optional: `tools/udev/99-ch341-ignore-modemmanager.rules` prevents ModemManager
from claiming CH340/CH341 adapters.

### OTA (bench)

MQTT-triggered A/B updates from a dev machine. See
[`tools/ota/README.md`](tools/ota/README.md).

```bash
./tools/ota/mqtt-ota.sh --device-id 768722 --skip-build
```

## Repository layout

```
spec/           Specifications (source of truth)
firmware/       Application source and board overlay
tools/          Bootstrap, SDK fetch, build, flash helpers
  web/          LAN admin UI sources, build script, host tests
  ota/          MQTT OTA bench scripts (see tools/ota/README.md)
external/       Gitignored — Airoha IoT SDK and Wine flash tool
```

Committed tree: `firmware/`, `spec/`, `tools/`, Unity test sources. The SDK is
not vendored.

## Safety

See [SAFETY.md](SAFETY.md). UART0 recovery on GPIO21/22.

## Non-affiliation

Not affiliated with Xiaomi, Mijia, or MediaTek. Product names identify
compatible hardware only. Independent firmware; no proprietary Xiaomi or
MediaTek binaries are redistributed.

## License

See repository root LICENSE. Third-party notices in [NOTICE](NOTICE).
