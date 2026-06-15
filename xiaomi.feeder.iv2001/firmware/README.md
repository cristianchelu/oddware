# Firmware

Implementation source for the pet feeder firmware. Built from the
specifications in `spec/`.

## Directory layout

```
firmware/
  GCC/                    # Makefile, linker script, feature.mk
  inc/                    # memory_map.h, ept_gpio_drv.h, FreeRTOS config
  src/                    # Application modules
  board/iv2001/           # BOARD_CONFIG=iv2001
  flash/                  # IV2001 flash_download.cfg (2 MB layout)
  patches/                # SDK patches (W25Q16DW flash combo, copy_firmware)
  test/                   # Host unit tests (Unity)
```

## Setup

```bash
./tools/fetch-sdk.sh
source tools/build-env.sh
```

`build-env.sh` creates the SDK app bridge under `project/mt7682_hdk/apps/petfeeder/`,
wires `BOARD_CONFIG=iv2001`, and applies patches from `patches/`.

## Host tests

```bash
make -C firmware/test test-host
```

## Target build

```bash
./tools/build-firmware.sh
```

Or:

```bash
source tools/build-env.sh
cd external/linkit-sdk-v4.7.1
./build.sh mt7682_hdk petfeeder bl
```

Build artifacts land in `external/LinkitSDK_OUT/mt7682_hdk/petfeeder/`.
`build-firmware.sh` also symlinks the `.bin` files into `firmware/flash/` next
to `flash_download.cfg` — the Flash Tool resolves ROM paths relative to that
config file, so open **`firmware/flash/flash_download.cfg`** after each build.
