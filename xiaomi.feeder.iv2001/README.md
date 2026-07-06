# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2**
(retail XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker. No
cloud, no MIoT, no phone-home.

## Firmware status

Replacement firmware is under active development. User goals live in
[`spec/20-stories/`](spec/20-stories/); detailed behavior in
[`spec/30-processes/`](spec/30-processes/).

### Build & recovery

- [x] IV2001 board build (MT7682, A/B partitions, custom bootloader)
- [x] Host unit tests (`make test-host`)
- [x] UART flash tooling (Linux)
- [x] MQTT OTA bench loop ([`tools/ota/`](tools/ota/))
- [x] UART development console (MiniCLI)
- [x] Remote telnet console (bench; optional `REMOTE_CLI_ENABLE`)

### Connectivity

- [x] Wi-Fi STA (stored credentials, connect lifecycle)
- [x] MQTT broker session (LWT on `connection`, device condition on `state`, reconnect backoff)
- [x] MQTT OTA command (`cmd/ota`)
- [ ] TLS
- [x] Home Assistant / MQTT integration [partial] (Dispense button, Bowl error sensor; remaining entities in spec)

### Provisioning

- [x] Captive portal on first boot (home Wi-Fi + MQTT via `PetFeeder-XXXX` AP)
- [x] Factory reset via UART CLI
- [ ] Re-provisioning via pin-hole reset (short press)
- [ ] Factory reset via pin-hole reset (long press)

### Feeding

- [x] Open-loop portion dispense (UART, MQTT, HA button)
- [x] Motor index tracking and jam / anti-jam
- [ ] Weight-based dispense (5–150 g target, bowl feedback)
- [ ] Dispense progress and outcome reporting over MQTT

### Sensing & display

- [x] Bowl weight on panel (default display mode)
- [x] Weight scale calibration (CLI: `weigh cal zero` / `span`)
- [x] Hopper level sensing (UART)
- [x] Status pictographs (Wi-Fi, MQTT, dispense)
- [ ] Bowl missing detection (Food Bowl Error)
- [ ] Alternate display modes (eaten today, off) from product paths

### Controls

- [x] Physical button input and gesture detection
- [x] Manual dispense button
- [x] Child lock [partial] (all physical gestures blocked except unlock combo; MQTT `cmd/config` pending)

### Power & battery

Goals are split across [`monitoring.md`](spec/20-stories/monitoring.md) (battery
reporting, conservation) and [`controls.md`](spec/20-stories/controls.md)
(sleep); mechanism in
[`power-state-machine.md`](spec/30-processes/power-state-machine.md) and
[`battery-monitoring.md`](spec/30-processes/battery-monitoring.md).

- [x] Mains vs battery source detection (UART)
- [ ] Power state machine (Normal / Battery / Sleep)
- [ ] Battery voltage and percentage
- [ ] Low-battery warning and conservation (display / Wi-Fi off; keep dispense)
- [ ] Wi-Fi on battery (`on` / `off` / `scheduled_only`)
- [ ] Sleep / wake (rear power button)

### Scheduling

- [ ] NTP time sync
- [ ] Schedule slots (MQTT CRUD, timed dispense)
- [ ] Next-feed reporting

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
