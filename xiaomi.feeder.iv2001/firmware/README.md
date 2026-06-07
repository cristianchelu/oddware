# Firmware

Implementation source for the pet feeder firmware. Built from the
specifications in `spec/` — never from proprietary sources.

## Status

**Not yet implemented.** Directory structure prepared for when specs reach
`stable` status.

## Planned layout

```
firmware/
├── apps/              # Application entry point, main task creation
├── board/iv2001/      # Board-specific config (pinmux, flash combo override)
├── src/               # Application modules (dispense, weigh, mqtt, etc.)
└── GCC/               # Makefile, linker script, startup
```

## Building

Requires the Airoha IoT SDK at `$SDK_ROOT`. Fetch it first:

```bash
./tools/fetch-sdk.sh
source tools/build-env.sh
```

Build commands TBD once implementation begins.
