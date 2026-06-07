# Flash hardware facts

## Flash chip

**Winbond W25Q16DW** — 16 Mbit / **2 MB**.
JEDEC ID: 0xEF 0x60 0x15. `[probe]` `[bootlog]`

The SDK flash-combo tables do not include this part by default — a custom
entry override is required for flash init to succeed. `[bootlog]`

## Chip characteristics


| Parameter    | Value                     | Source          |
| ------------ | ------------------------- | --------------- |
| Capacity     | 2 MB (16 Mbit)            | `[ds:W25Q16DW]` |
| Sector size  | 4 KB                      | `[ds:W25Q16DW]` |
| Block size   | 64 KB                     | `[ds:W25Q16DW]` |
| Page size    | 256 bytes                 | `[ds:W25Q16DW]` |
| Interface    | SPI / Dual-SPI / Quad-SPI | `[ds:W25Q16DW]` |
| Erase cycles | 100K per sector typical   | `[ds:W25Q16DW]` |


## Identity and calibration

- **MAC address:** sourced from **efuse** (hardware OTP), not flash. `[bootlog]`
- **RF calibration:** no dedicated immutable RF cal partition. Country/region
and channel tables are set at runtime by firmware. `[bootlog]`
- **Full-chip erase** does not destroy device identity or RF trim.

## What this means for implementers

- **2 MB is fully ours.** Bootloader, application, OTA staging, config
storage — the entire flash layout is ours to define. The SDK provides
bootloader source; we build our own.
- **No sacred regions.** There is no irreplaceable factory data in flash
that must be preserved. MAC is in efuse; WiFi regulatory config is
runtime. A clean chip-erase + full reprogram is a valid workflow.
- **Flash combo override required.** The W25Q16DW JEDEC ID must be added
to the SDK's flash-combo table at build time or the flash init will fail.
- **Recovery:** the MT7682 boot ROM provides a UART0 (GPIO21/22) download
mode regardless of flash contents. Even a fully erased chip can be
programmed via the MediaTek IoT Flash Tool. `[bootlog]`

## UART recovery path


| Parameter       | Value                                | Source                |
| --------------- | ------------------------------------ | --------------------- |
| UART            | UART0 — GPIO21 (TX), GPIO22 (RX)     | `[bootlog]` `[probe]` |
| Baud (ROM mode) | 115200                               | `[bootlog]`           |
| Tool            | MediaTek IoT Flash Tool              | `[bootlog]`           |
| Trigger         | Boot ROM always available (hardware) | `[ds:MT7682]`         |


This is the brick-recovery path. As long as you can reach the UART0 pads,
the chip can always be reflashed from scratch.