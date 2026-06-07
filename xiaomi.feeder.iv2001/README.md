# @oddware/xiaomi.feeder.iv2001

Open-source replacement firmware for the **Xiaomi Smart Pet Food Feeder 2** hardware
(retail model XMWSQ02, cloud model `xiaomi.feeder.iv2001`).

Local-only MQTT control for Home Assistant, Homey, or any MQTT broker. No cloud,
no MIoT, no phone-home.

## Status

**Early specification phase.** No runnable firmware yet — we are writing the hardware
and behavioral specs first, then implementing from those specs.

## Non-affiliation disclaimer

This project is **not** affiliated with, endorsed by, or associated with Xiaomi, Mijia,
MediaTek, or Airoha in any way. Product names are used **descriptively** to identify
compatible hardware only — no ownership or sponsorship is implied.

The firmware produced here is an independent work. It does not contain, redistribute,
or derive from any proprietary Xiaomi or MediaTek firmware binary.

## Building

The firmware builds against the Airoha IoT SDK (MT7682 support). The SDK is **not**
included in this repository. Fetch it once:

```bash
./tools/fetch-sdk.sh
```

This clones the SDK into `external/airoha-iot-sdk/` (gitignored). Then:

```bash
source tools/build-env.sh
# (build instructions TBD once firmware skeleton exists)
```

## Repository layout

```
spec/           Specification documents (hardware facts + our design)
firmware/       Implementation source (C / FreeRTOS), built against $SDK_ROOT
tools/          Build helpers, SDK fetch
external/       Gitignored: fetched SDK lives here at build time
```

## Safety

Flashing custom firmware can **brick your device** or create **fire/electrical hazards**
if done incorrectly. Read [SAFETY.md](SAFETY.md) before you begin. Do not sell
pre-flashed devices.

## License

See the repository root LICENSE (oddware monorepo). Third-party notices in [NOTICE](NOTICE).