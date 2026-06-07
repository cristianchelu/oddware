# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2** hardware
(retail model XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker. No cloud,
no MIoT, no phone-home.

## Status

Early development. Specifications and build tooling target LinkIt SDK 4.6 on
MT7682 (no PSRAM). Application firmware is not yet complete.

## Quick start

From this directory (`xiaomi.feeder.iv2001/`):

```bash
./tools/bootstrap.sh
```

Checks prerequisites, runs host unit tests, fetches the LinkIt SDK into
gitignored `external/`, and runs `build-env.sh`.

Host tests only (no SDK fetch):

```bash
./tools/bootstrap.sh --host-only
# or
make test-host
```

### Prerequisites

| Tool | Needed for | Fedora | Debian/Ubuntu |
|------|------------|--------|---------------|
| `git`, `make`, `gcc` | Host tests | `dnf install git make gcc` | `apt install git make gcc` |
| `patch` | SDK patches | `dnf install patch` | `apt install patch` |
| `arm-none-eabi-gcc` | Board firmware | `dnf install arm-none-eabi-gcc-cs` | `apt install gcc-arm-none-eabi` |

The SDK clone is large; use `--host-only` when working on specs or host tests only.

### Firmware build

```bash
source tools/build-env.sh
./tools/build-firmware.sh
```

Requires `firmware/GCC/Makefile` and related scaffold (see `firmware/README.md`).

## Non-affiliation disclaimer

This project is **not** affiliated with, endorsed by, or associated with Xiaomi,
Mijia, or MediaTek in any way. Product names are used **descriptively** to
identify compatible hardware only.

The firmware is an independent work. It does not contain, redistribute, or
derive from proprietary Xiaomi or MediaTek firmware binaries.

## Repository layout

```
spec/           Specifications (hardware + design — source of truth)
firmware/       Oddware C/FreeRTOS implementation (committed)
tools/          bootstrap, fetch-sdk, build-env, build-firmware
external/       Gitignored — LinkIt SDK fetched here
```

Only `firmware/`, `spec/`, `tools/`, and vendored Unity test sources are committed.
The SDK is not part of the repository.

## Safety

Read [SAFETY.md](SAFETY.md) before flashing. UART0 recovery @ GPIO21/22.

## License

See repository root LICENSE. Third-party notices in [NOTICE](NOTICE).
