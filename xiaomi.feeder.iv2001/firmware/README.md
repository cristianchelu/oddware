# Firmware

Implementation source for the pet feeder firmware. Built from the
specifications in `spec/` — never from proprietary sources.

## Directory layout

```
firmware/
  GCC/                    # Makefile, linker script, feature.mk
  inc/                    # FreeRTOS config, task_def.h, memory_map.h
  src/                    # Application modules
  src/fota_dual/          # Adapted fota_dual_image from houndify SDK
  ports/                  # Port interface headers
  adapters/               # SDK adapter implementations
  test/                   # Host-side unit tests (Unity)
  test/fakes/             # Fake port implementations for host tests
  patches/                # SDK patches (flash combo W25Q16DW)
```

## Prerequisites

```bash
./tools/fetch-sdk.sh
source tools/build-env.sh
```

`build-env.sh` creates the SDK symlink (`apps/petfeeder` → this tree)
and applies patches from `patches/`.

## Host tests

Fast TDD loop — no hardware required:

```bash
make -C firmware/test test-host
```

## Target build

Once `GCC/Makefile` exists (Step 1):

```bash
cd external/airoha-iot-sdk
./build.sh aw7698_evk petfeeder bl
```
