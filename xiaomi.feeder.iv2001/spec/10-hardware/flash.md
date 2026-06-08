# Flash hardware facts

## Flash chip

**Winbond W25Q16DW** — 16 Mbit / **2 MB**.
JEDEC ID: 0xEF 0x60 0x15. `[probe]` `[bootlog]`

The SDK flash-combo tables do not include this part by default — a custom
entry override is required for flash init to succeed. `[bootlog]`

## Chip characteristics

Verified against Winbond W25Q16DW datasheet (Rev J, Sep 2014)
`[ds:W25Q16DW]`.

| Parameter    | Value                     | Source          |
| ------------ | ------------------------- | --------------- |
| Capacity     | 2 MB (16 Mbit, 2,097,152 B) | `[ds:W25Q16DW]` §1–2 |
| Supply       | 1.65–1.95 V (1.8 V family)  | `[ds:W25Q16DW]` §2 |
| JEDEC ID     | MFID `EFh`, Device `6015h` (bytes `0xEF 0x60 0x15`) | `[ds:W25Q16DW]` §7.2.1 |
| Organization | 8,192 pages; 512 × 4 KB sectors; 32 × 64 KB blocks | `[ds:W25Q16DW]` §1 |
| Page size    | 256 bytes (program 1–256 B per command) | `[ds:W25Q16DW]` §1–2, §7.2.20 |
| Sector erase | 4 KB uniform              | `[ds:W25Q16DW]` §1–2 |
| Block erase  | 32 KB and 64 KB uniform   | `[ds:W25Q16DW]` §1–2 |
| Interface    | Standard / Dual / Quad SPI, QPI | `[ds:W25Q16DW]` §2 |
| Endurance    | >100,000 erase/program cycles | `[ds:W25Q16DW]` §2 |
| Data retention | >20 years (typ.)        | `[ds:W25Q16DW]` §2 |

### Page program rules (OTA-relevant)

- Programs only into **previously erased (`FFh`)** locations (§7.2.20).
- **Partial page program** is allowed: fewer than 256 bytes can be
  programmed without affecting other bytes in the same page, provided the
  command does not wrap past the page boundary (§7.2.20).
- Sending **>256 bytes** in one Page Program wraps within the page and
  overwrites earlier bytes in that command (instruction-set notes, §7.2).

Our OTA path: 4 KB sector erase ahead, then programs up to **256 bytes**
per `hal_flash_write` (one Winbond page). The MediaTek SFC driver issues
**up to two 128-byte partial programs** per page internally (GPRAM limit);
that is datasheet-compliant partial page programming, not an arbitrary
chunk size.


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

## Recommended UART flash harness

Use a **3.3 V** USB-TTL adapter (not 5 V without level shifting).

| Adapter signal | Feeder connection | Notes |
| -------------- | ----------------- | ----- |
| TX | TP2 (module RX / GPIO22) | Crossed pair with RX |
| RX | TP1 (module TX / GPIO21) | |
| GND | TP27 | Common ground |

`CHIP_EN` (tapped at **TP15**, same net as unpopulated **SW3**) is active-high:
low holds the module in reset; releasing high while the flash tool is already
listening and sending BROM sync (`0xA0`) enters download mode.

### Manual reset workflow

`tools/iot-flash.sh` starts the Wine flash tool (CODA or GUI), waits for it to
open the COM port, then prompts you to reset the feeder within
`IOT_FLASH_RESET_WAIT_SEC` (default 30s, set in `tools/iot-flash.env`):

- **Power-cycle** the supply, or
- **Pulse TP15** (SW3 RESET / CHIP_EN)

Do not hold CHIP_EN low while the tool is syncing — the ROM is off and TX will
spam with no `0x5F` response.

**SW1** (BOOT strap, **TP14**) is not required for UART0 BROM download on this board.

### Linux tooling

`tools/setup-flashtool.sh` extracts the IoT Flash Tool from the LinkIt SDK
`PC_tool.zip` into gitignored `external/`. `tools/iot-flash.sh` maps the
Linux tty to Wine `COM3`.

**What works on Linux (Wine + CODA):**

| Operation | Command | Notes |
| --------- | ------- | ----- |
| Download | `iot-flash.sh download DEVICE` | CFG-only; manual reset prompt |
| BROM probe | `iot-flash.sh probe DEVICE` | Native `0xA0`/`0x5F` sync, no Wine |
| UART console | `uart-console.sh DEVICE` | `UART_CONSOLE_SECS=N` or picocom |

**Readback is not available headlessly on Linux.** CODA `-r` with an INI file
fails under Wine (`Failed to parse the section: [settings]`). CFG-only `-r`
hangs with no output. Use the Windows IoT Flash Tool for full-chip readback
(`firmware/flash/coda_readback_2mb.ini` is a Windows reference template).
