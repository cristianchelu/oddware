# Build integration and test infrastructure

## SDK build system

The Airoha IoT SDK uses a `build.sh` script that expects projects at:

```
project/aw7698_evk/apps/<name>/GCC/Makefile
```

Our firmware source lives at `xiaomi.feeder.iv2001/firmware/`, outside the
SDK tree. A symlink bridges the two:

```
external/airoha-iot-sdk/project/aw7698_evk/apps/petfeeder
  -> <repo>/firmware/    (absolute path; created by build-env.sh)
```

The symlink is created by `tools/build-env.sh` and lives under `external/`
(gitignored). `[design]`

## Onboarding (new clone)

From `xiaomi.feeder.iv2001/`:

```
./tools/bootstrap.sh
```

Steps performed:

1. Verify host tools (`git`, `make`, `gcc`); `patch` for full bootstrap.
2. Run `make test-host` — proves the vendored Unity harness works.
3. Clone the SDK via `tools/fetch-sdk.sh` (skipped with `--host-only`).
4. Run `tools/build-env.sh` — symlink + patch application.

Host tests do not require the SDK.

### Build command

```
cd external/airoha-iot-sdk
./build.sh aw7698_evk petfeeder bl
```

The `bl` suffix builds the bootloader alongside the application.

## Firmware directory structure

```
firmware/
  GCC/                    # Makefile, linker script, feature.mk
  inc/                    # FreeRTOS config, task_def.h, memory_map.h
  src/                    # Application modules
  src/fota_dual/          # Adapted fota_dual_image from houndify SDK
  ports/                  # Port interface headers
  adapters/               # SDK adapter implementations
  test/                   # Host-side test source
  test/fakes/             # Fake port implementations
  patches/                # SDK patches (flash combo W25Q16DW)
```

## SDK patches

### Flash combo table (W25Q16DW)

Neither SDK includes the Winbond W25Q16DW in its flash combo table. The
JEDEC ID `0xEF, 0x60, 0x15` must be added to
`driver/chip/aw7698/src/hal_flash_combo_nor.c`.

Maintained as `firmware/patches/flash_combo_w25q16dw.patch`, applied
automatically by `tools/build-env.sh`. The patch adds a single combo table
entry. Without it the SDK refuses to initialize flash.

## `tools/build-env.sh`

This script prepares the build environment:

1. Creates the SDK symlink (if not already present).
2. Applies patches from `firmware/patches/` (idempotent).
3. Verifies the toolchain is accessible.

Run once after cloning, or after `tools/fetch-sdk.sh`.

## Test-driven development

All firmware development follows strict red/green/refactor TDD. `[design]`

### Host tests (`make test-host`)

Compiled with host gcc/clang. Test application logic through fake port
implementations. Fast, runs locally via `make test-host`.

- **Framework:** [Unity](https://github.com/ThrowTheSwitch/Unity) (C-only,
  zero dependencies, ~3 source files).
- **Location:** `firmware/test/test_*.c`
- **Fakes:** `firmware/test/fakes/fake_*.c`
- **Build:** `make -C firmware/test test-host`

Host tests assert Tier 3 process behaviors through port contracts. They
have no FreeRTOS dependency -- the event queue is replaced with a simple
FIFO array in test harness code.

### On-target integration tests

Compiled into the firmware image. Triggered via UART CLI command or MQTT
topic. Assert real HAL behavior: flash writes, WiFi association, MQTT
pub/sub.

Run manually with the board on the bench. Used to validate that adapters
correctly implement their port contracts against real hardware.
