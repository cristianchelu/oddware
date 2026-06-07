# SDK reference

Both SDKs are under `external/`, fetched by `tools/fetch-sdk.sh`.

## SDK inventory

| SDK | Path | Version | FreeRTOS | Chip config | Role |
|-----|------|---------|----------|-------------|------|
| **Airoha IoT SDK** | `external/airoha-iot-sdk/` | V4.9.0 | 10.1.1 | `aw7698` (`PRODUCT_VERSION=7698`) | **Primary build SDK** |
| **LinkIt SDK (houndify)** | `external/linkit-sdk-v4.6.2-houndify/` | V4.6.2 | 8.2.0 | `mt7682` (`PRODUCT_VERSION=7682`) | **Read-only reference** |

Both target the same silicon family: AW7698 is an Airoha rebrand of
MT7682/MT7686.

## Airoha IoT SDK (primary)

All builds run against this tree. It provides:

- Linux GCC toolchain (ARM cross-compiler)
- AW7698-specific HAL drivers
- Prebuilt WiFi stack (`libwifi_aw7698_ram.a`, also ships `libwifi_mt7682_ram.a`)
- FreeRTOS 10.1.1
- Middleware: MQTT client, HTTP client/server, NVDM, mbedTLS, FOTA

### Reference applications

| App | Path | Relevance |
|-----|------|-----------|
| `bootloader` | `project/aw7698_evk/apps/bootloader/` | Bootloader base, copy-down FOTA model |
| `fota_over_wifi` | `project/aw7698_evk/apps/fota_over_wifi/` | FOTA example, feature.mk template, memory_map.h reference |
| `mqtt_client` | `project/aw7698_evk/apps/mqtt_client/` | MQTT client usage pattern |
| `wifi_demo` | `project/aw7698_evk/apps/wifi_demo/` | WiFi STA/AP mode, CLI config pattern |
| `httpd` | `project/aw7698_evk/apps/httpd/` | HTTP server for captive portal |

### FOTA model limitation

The airoha SDK implements only a **copy-down** FOTA model for AW7698: a
single application slot plus a staging area. The bootloader copies the
staged image over the active slot on boot. This does not provide true A/B
redundancy. `[design]`

## LinkIt SDK v4.6.2 (houndify, read-only reference)

Not used for building. Provides reference code and MT7682-specific
configurations:

- `mt7682_hdk` board project with MT7682-specific configs
- Flash download configuration for MT7682 (`flash_download.cfg`)
- Dual-image FOTA implementation with A/B bank switching

### Key reference files

| File | Path | What we take from it |
|------|------|----------------------|
| `fota_dual_image.c` | `middleware/MTK/fota/src/internal/` | A/B control block logic, bank switching, SHA-512 verification |
| `fota_dual_image.h` | `middleware/MTK/fota/inc/internal/` | Control block struct, API signatures |
| `flash_map_dual.h` | `middleware/MTK/fota/inc/internal/` | Partition address definitions (adapted to our 2 MB layout) |
| `bl_fota.c` | `project/mt7682_hdk/apps/bootloader/src/` | Bootloader FOTA integration example |

### Dual-image FOTA adaptation

The three dual-image files (~300 lines total) are **copied and adapted**
into `firmware/src/fota_dual/` rather than cross-included at build time.

Reason: `fota_dual_image.c` depends on `fota_internal.h` whose struct
layouts differ between the houndify SDK (MT7682-era) and the airoha SDK
(AW7698-era). Mixing include paths from two SDKs risks silent ABI
mismatches. By copying the source and replacing `fota_internal.h` calls
with direct `hal_flash_*` calls from the airoha SDK, we get a
self-contained, auditable module. `[design]`

The adaptation changes:

- Flash addresses in `flash_map_dual.h` updated to the 2 MB layout (see
  [partition-layout.md](partition-layout.md)).
- Internal flash read/write/erase calls replaced with airoha SDK
  `hal_flash_*` equivalents.
- SHA-512 verification retained; the houndify code has separate N9/CM4 hash
  fields, but AW7698 uses a combined single image, so one hash field covers
  the full bank contents.
- Control block format preserved: magic `0x4455414C` ("DUAL"), active flag
  `FOTA_IMAGE_A_MARK` = `0xABCDDCBA`, `FOTA_IMAGE_B_MARK` = `~0xABCDDCBA`.
