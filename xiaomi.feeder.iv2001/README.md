# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2**
(retail XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker. No
cloud, no MIoT, no phone-home.

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
| `patch` | SDK patches | `dnf install patch` | `apt install patch` |
| `arm-none-eabi-gcc` + newlib | Board firmware | `dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib` | `apt install gcc-arm-none-eabi` |
| `wine`, `unzip` | Linux UART flash | `dnf install wine unzip` | `apt install wine unzip` |

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
