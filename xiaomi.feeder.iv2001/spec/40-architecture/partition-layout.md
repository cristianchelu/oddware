# Flash partition layout

## Physical flash

Winbond W25Q16DW: **2 MB** (2,097,152 bytes), SPI NOR, JEDEC `0xEF 0x60
0x15`. Mapped at `0x08000000`–`0x08200000`. See
[flash.md](../10-hardware/flash.md). `[probe]`

## SDK default layout (4 MB, does not fit)

The SDK's linker scripts and `memory_map.h` assume 4 MB:

| Region | Base | Size |
|---|---|---|
| HEAD_1 | 0x08000000 | 4 KB |
| HEAD_2 | 0x08001000 | 4 KB |
| Bootloader | 0x08002000 | 64 KB |
| CM4 (app) | 0x08012000 | 2344 KB |
| FOTA reserved | 0x0825C000 | 1612 KB |
| NVDM | 0x083EF000 | 64 KB |
| WiFi TX power | 0x083FF000 | 4 KB |

This layout must be replaced entirely. `[design]`

## IV2001 partition table (2 MB, A/B dual-bank)

```
Address        Region         Size     Notes
0x08000000     HEAD_1          4 KB    N9 header (reserved by SDK)
0x08001000     HEAD_2          4 KB    CM4 header (reserved by SDK)
0x08002000     Bootloader     64 KB    Boot selector + A/B control block
0x08012000     Bank A        952 KB    Active firmware (default)
0x08100000     Bank B        952 KB    Inactive / OTA target
0x081EE000     NVDM           64 KB    Config storage (WiFi, MQTT creds)
0x081FE000     TX Power        4 KB    WiFi TX power calibration data
0x081FF000     (reserved)      4 KB    Pad to 2 MB boundary
0x08200000     --- end of flash ---
```

952 KB per bank. The SDK's FOTA WiFi example compiles to ~700–800 KB,
leaving headroom for application features. `[design]`

## A/B control block

Located at flash offset `0xF000` (absolute `0x0800F000`), in the last
4 KB sector of the 64 KB bootloader region — above the linked bootloader
code. The slot at `0x8000` (`0x08008000`) falls inside linked bootloader
`.text` and is not used for the control block.
Format matches LinkIt SDK dual-image FOTA (see
[sdk-reference.md](sdk-reference.md)):

| Field | Offset | Size | Value |
|-------|--------|------|-------|
| Magic | 0x00 | 4 B | `0x4455414C` ("DUAL") |
| Active flag | 0x04 | 4 B | `0xABCDDCBA` (Bank A) or `0x54322345` (Bank B) |
| SHA-512 hash | 0x08 | 64 B | Image integrity hash of active bank |
| (reserved) | 0x48 | — | Pad to sector boundary |

The bootloader reads the control block before jumping. Logic:

| Condition | Boot target |
|-----------|-------------|
| Magic valid, flag = A mark | Bank A (`0x08012000`) |
| Magic valid, flag = B mark | Bank B (`0x08100000`) |
| Magic invalid or flag = 0xFF (erased flash) | Bank A (safe default) |

## Regions detail

### HEAD_1 / HEAD_2

SDK-reserved headers for N9 and CM4 firmware metadata. Written by the SDK
build tools during image generation. Not modified by application code.

### Bootloader

Custom bootloader based on the LinkIt SDK `bootloader` example. Reads the
A/B control block and jumps to the active bank via `fota_dual_image`.

### Bank A / Bank B

Each bank holds a complete application firmware image. During OTA, the
download writes to the inactive bank while the active bank continues
running. On successful verification, the control block's active flag is
flipped and the device reboots.

### NVDM

Non-Volatile Data Management region. Stores WiFi credentials, MQTT broker
config, schedule data, calibration values, and tunable parameters.

### TX Power

WiFi TX power calibration data. Written once during manufacturing or
initial setup. Referenced by the WiFi driver at boot.

## Flash combo table

The stock flash combo table omits W25Q16DW (JEDEC `0xEF, 0x60, 0x15`). Add it
via `firmware/patches/flash_combo_w25q16dw.patch` during build setup. See
[build-integration.md](build-integration.md). `[design]`
