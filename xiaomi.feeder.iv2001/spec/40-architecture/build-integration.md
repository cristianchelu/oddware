# Build integration and test infrastructure

## Repository rules

1. **SDK stays out of git.** `external/` is gitignored. The LinkIt SDK is
   cloned locally by `tools/fetch-sdk.sh`. Never commit SDK files, prebuilt
   libraries, or vendored example app sources. Patches and oddware firmware
   only.
2. **IV2001 board config is committed.** `BOARD_CONFIG=iv2001` with pinmux in
   `firmware/inc/ept_gpio_drv.h` and `firmware/src/ept_gpio_var.c`. Do not
   rely on `mt7682_hdk` EVK pinmux — it conflicts with feeder peripherals.
3. **MT7682, no PSRAM.** `IC_CONFIG=mt7682`, `MTK_NO_PSRAM_ENABLE=y`.
   PSRAM-enabled builds are incompatible with the MHCW05P-B module.
4. **Scaffold by path.** SDK example files (`sys_init.c`, `startup_mt7682.s`,
   etc.) are referenced from the fetched tree at build time, not copied into
   `firmware/`.

## SDK layout

LinkIt SDK `build.sh` expects:

```
project/mt7682_hdk/apps/<name>/GCC/Makefile
```

Fetched to: `external/linkit-sdk-v4.6.2-houndify/` (gitignored).

Oddware firmware lives at `xiaomi.feeder.iv2001/firmware/`. `build-env.sh`
creates a real directory (not a symlink — `build.sh` does not follow symlinked
apps):

```
external/.../project/mt7682_hdk/apps/petfeeder/
  src/   inc/   flash/   GCC/   → symlinks into <repo>/firmware/
```

## IV2001 board overlay

| Component | Committed | SDK bridge (gitignored) |
|-----------|-----------|-------------------------|
| `board.mk` | `firmware/board/iv2001/` | `config/board/iv2001/` symlink |
| Pinmux | `firmware/inc/ept_gpio_drv.h`, `src/ept_gpio_var.c` | — |
| Console BSP | — | `driver/board/iv2001/util` → `mt7682_hdk/util` |
| EPT applier | — | `driver/board/iv2001/ept` → `mt7686_hdk/ept` |
| Chip HAL | — | `driver/chip/mt7686/module.mk` (always) |

`build-env.sh` creates board symlinks and applies `firmware/patches/`.

## Onboarding

From `xiaomi.feeder.iv2001/`:

```
./tools/bootstrap.sh           # host tests + SDK fetch + build-env
./tools/bootstrap.sh --host-only   # host tests only
```

Prerequisites: `git`, `make`, `gcc`, `patch` (for SDK patches),
`arm-none-eabi-gcc` (for target builds).

### Build

```
source tools/build-env.sh
cd external/linkit-sdk-v4.6.2-houndify
./build.sh mt7682_hdk petfeeder bl
```

Or: `./tools/build-firmware.sh`

Flash package: `external/linkit-sdk-v4.6.2-houndify/out/mt7682_hdk/petfeeder/`

`firmware/flash/flash_download.cfg` supplies IV2001 2 MB addresses for the
MediaTek Flash Tool (overrides the SDK default 1 MB layout).

## Firmware directory structure

```
firmware/
  board/iv2001/           # board.mk
  GCC/                    # Makefile, mt7682_flash.ld, feature.mk
  inc/                    # memory_map.h, ept_gpio_drv.h
  src/                    # main.c, ept_gpio_var.c, app modules
  flash/                  # flash_download.cfg (IV2001 2 MB)
  patches/                # SDK patches (W25Q16DW combo)
  ports/                  # Port headers
  adapters/               # SDK adapters
  test/                   # Host tests + vendored Unity
```

## SDK patches

### Flash combo (W25Q16DW)

JEDEC `0xEF, 0x60, 0x15` is missing from the default `driver/chip/mt7686/`
combo headers. Without it, `hal_flash_init()` fails on IV2001 hardware.

Maintained as `firmware/patches/flash_combo_w25q16dw.patch`, applied by
`build-env.sh` (idempotent).

### Wi-Fi NVDM namespaces

The SDK `wifi_nvdm_config` module seeds its own NVDM groups (`STA`, `AP`,
`common`, …) for radio defaults. Application Wi-Fi credentials use a
separate `wifi` group (`ssid`, `pass`) per
[config-store.md](../30-processes/config-store.md). `sta_auto_connect` is
disabled in `wifi_adapter_stack_init()` so the SDK profile is not used for
association; the connect task reads only the `wifi` group.

## `tools/build-env.sh`

1. Symlink `petfeeder` app bridge under `project/mt7682_hdk/apps/`.
2. Symlink IV2001 board config into SDK `config/` and `driver/board/`.
3. Apply patches from `firmware/patches/`.
4. Wire `arm-none-eabi-gcc` into PATH and SDK-expected symlink paths.

## Test-driven development

Strict red/green/refactor. `[design]`

### Host tests

```
make test-host
```

Unity (vendored in `firmware/test/unity/`). Fakes in `firmware/test/fakes/`.
No FreeRTOS dependency.

### On-target tests

Cross-compiled; triggered via UART CLI (see
[uart-console.md](../30-processes/uart-console.md)) or MQTT. Validate
adapters against real HAL.
