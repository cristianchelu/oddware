# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2** hardware
(retail model XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker. No cloud,
no MIoT, no phone-home.

## Status

**Firmware foundation in progress.** Specs are written; host unit tests and build
scaffolding exist. Target firmware build (Step 1) is next.

## Quick start (new clone)

From this directory (`xiaomi.feeder.iv2001/`):

```bash
./tools/bootstrap.sh
```

That checks prerequisites, runs host unit tests, clones the Airoha SDK into
`external/`, and configures the SDK symlink plus patches.

**Host tests only** (no SDK clone — fast sanity check):

```bash
./tools/bootstrap.sh --host-only
# or
make test-host
```

### Prerequisites


| Tool                 | Needed for                   | Fedora                             | Debian/Ubuntu                   |
| -------------------- | ---------------------------- | ---------------------------------- | ------------------------------- |
| `git`, `make`, `gcc` | Host tests                   | `dnf install git make gcc`         | `apt install git make gcc`      |
| `patch`              | Full bootstrap (SDK patches) | `dnf install patch`                | `apt install patch`             |
| `arm-none-eabi-gcc`  | Board firmware (Step 1+)     | `dnf install arm-none-eabi-gcc-cs` | `apt install gcc-arm-none-eabi` |


Host development needs only the first row. The SDK clone is large (~1 GB);
skip it with `--host-only` if you are only working on specs or host-tested logic.

### After bootstrap

```bash
make test-host                  # re-run unit tests anytime
source tools/build-env.sh       # per-shell ARM toolchain PATH
```

Target build (once `firmware/GCC/Makefile` exists):

```bash
cd external/airoha-iot-sdk
./build.sh aw7698_evk petfeeder bl
```

## Non-affiliation disclaimer

This project is **not** affiliated with, endorsed by, or associated with Xiaomi, Mijia,
MediaTek, or Airoha in any way. Product names are used **descriptively** to identify
compatible hardware only — no ownership or sponsorship is implied.

The firmware produced here is an independent work. It does not contain, redistribute,
or derive from any proprietary Xiaomi or MediaTek firmware binary.

## Repository layout

```
spec/           Specification documents (hardware facts + our design)
firmware/       Implementation source (C / FreeRTOS), built against $SDK_ROOT
tools/          bootstrap.sh, fetch-sdk.sh, build-env.sh
external/       Gitignored: fetched SDK lives here at build time
```

## Safety

Flashing custom firmware can **brick your device** or create **fire/electrical hazards**
if done incorrectly. Read [SAFETY.md](SAFETY.md) before you begin. Do not sell
pre-flashed devices.

## License

See the repository root LICENSE (oddware monorepo). Third-party notices in [NOTICE](NOTICE).