# SDK reference

## Silicon and SDK

IV2001 uses **MT7682** in the Xiaomi MHCW05P-B module. There is no external
PSRAM on this module.

All firmware builds use **Airoha IoT SDK v4.7.1** (demoMT mirror):

| Property | Value |
|----------|-------|
| Path | `external/linkit-sdk-v4.7.1/` (gitignored, fetched locally) |
| Upstream | `https://github.com/dangkhoalk95/demoMT` (`master`, pinned commit in `tools/fetch-sdk.sh`) |
| Board family | `mt7682_hdk` |
| `IC_CONFIG` | `mt7682` |
| `PRODUCT_VERSION` | `7682` (via `config/chip/mt7682/chip.mk`) |
| Chip HAL module | `driver/chip/mt7686/module.mk` |
| FreeRTOS | 8.2.0 (version is not architecturally significant) |

The SDK is not committed to this repository. See
[build-integration.md](build-integration.md).

## Module memory profile

LinkIt `fota_over_wifi` is the reference app profile for this hardware:

```
MTK_NO_PSRAM_ENABLE                 = y
MTK_MEMORY_WITHOUT_PSRAM            = y
MTK_MEMORY_WITH_PSRAM_FLASH         = n
```

Linker script must place `.data`/`.bss` in SYSRAM/TCM — not PSRAM `RAM`/`VRAM`
regions. Our `firmware/GCC/mt7682_flash.ld` extends the SDK no-PSRAM template
with the IV2001 2 MB flash bank layout.

## What the SDK provides

- MT7682 HAL and drivers (`driver/chip/mt7686/`)
- Prebuilt WiFi: `libwifi_mt7682_ram.a`
- Middleware: NVDM, MQTT, HTTP client, mbedTLS, **dual-image FOTA**
- Example apps under `project/mt7682_hdk/apps/`

### Reference applications (read-only scaffold — not committed)

| App | Path | Use |
|-----|------|-----|
| `bootloader` | `project/mt7682_hdk/apps/bootloader/` | Boot + dual-image jump |
| `fota_over_wifi` | `project/mt7682_hdk/apps/fota_over_wifi/` | `feature.mk`, `Makefile`, `sys_init.c` template |
| `mqtt_client` | `project/mt7682_hdk/apps/mqtt_client/` | MQTT patterns |
| `wifi_demo` | `project/mt7682_hdk/apps/wifi_demo/` | WiFi STA / CLI NVDM |

The application `Makefile` compiles oddware sources plus **paths into** these
examples (e.g. `sys_init.c`, `startup_mt7682.s`). Example sources are not
copied into git.

## Dual-image FOTA

LinkIt ships `middleware/MTK/fota/src/internal/fota_dual_image.c` and
`flash_map_dual.h`. Enable `MTK_FOTA_DUAL_IMAGE_ENABLE` in `feature.mk`.

Adapt partition addresses in `flash_map_dual.h` (or a local override) to match
[partition-layout.md](partition-layout.md) — 2 MB, Bank A/B, control block at
`0x08008000`.

## Toolchain

LinkIt does not bundle a Linux `arm-none-eabi-gcc`. Use a distro package
(`gcc-arm-none-eabi`, `arm-none-eabi-gcc-cs`) or a standalone ARM GNU Toolchain
install. `build-env.sh` puts it on PATH and symlinks
`tools/gcc/linux/gcc-arm-none-eabi` inside the SDK tree (expected by Makefiles).

## IV2001 overlays (committed in `firmware/`)

| File | Purpose |
|------|---------|
| `board/iv2001/board.mk` | `BOARD_CONFIG=iv2001` |
| `inc/ept_gpio_drv.h`, `src/ept_gpio_var.c` | IV2001 pinmux |
| `inc/memory_map.h` | 2 MB partition constants |
| `GCC/mt7682_flash.ld` | No-PSRAM linker + 2 MB banks |
| `flash/flash_download.cfg` | MediaTek Flash Tool, 2 MB addresses |
| `patches/flash_combo_w25q16dw.patch` | W25Q16DW in `driver/chip/mt7686/` |

## UART

Console, flash tool, and recovery: **UART0 @ GPIO21/22**, 115200 8N1.
UART1 (GPIO2/3) is factory-only — leave disabled in application firmware.
