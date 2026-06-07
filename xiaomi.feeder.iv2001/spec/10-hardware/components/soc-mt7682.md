# MT7682 — Wi-Fi SoC

## Summary

Single-chip ARM Cortex-M4F with integrated 802.11 b/g/n (2.4 GHz), flash
controller, PMU, and peripherals. Runs FreeRTOS via the Airoha IoT SDK.
Packaged inside the MHCW05P-B module on this board.

## Key specs

| Parameter | Value | Source |
|-----------|-------|--------|
| Core | Cortex-M4F, up to 192 MHz | `[ds:MT7682]` |
| Flash | 2 MB external NOR (W25Q16DW on this board) | `[probe]` `[bootlog]` |
| RAM | 352 KB SRAM | `[ds:MT7682]` |
| Wi-Fi | 802.11 b/g/n, 2.4 GHz, STA+AP | `[ds:MT7682]` |
| GPIO | 23 pins (GPIO0–GPIO22) | `[ds:MT7682]` |
| ADC | 1 channel, 12-bit, 2500 mV reference | `[ds:MT7682]` |
| UART | 3 ports (UART0/1/2) | `[ds:MT7682]` |
| I2C | 2 masters | `[ds:MT7682]` |
| SPI | master + slave | `[ds:MT7682]` |

## Peripherals used on this board

- **UART0** (GPIO21/22): boot ROM, flash programming
- **UART1** (GPIO2/3): factory test (not used in production firmware)
- **UART2** (GPIO11/12): CS1270 weighing ASSP
- **I2C1** (GPIO15/16): AW9523B GPIO expander
- **ADC ch0** (GPIO17): battery/motor sense via analog mux
- **EINT** (GPIO4): AW9523B interrupt
- **GPIO bit-bang** (GPIO1, GPIO13): TM1637 display

## Flash chip

The module uses a **Winbond W25Q16DW** (JEDEC: 0xEF 0x60 0x15) — 16 Mbit /
2 MB, fully populated and entirely available for our use (we build our own
bootloader). The stock SDK flash-combo tables do not include this part — a
custom combo entry is required. `[bootlog]` `[probe]`

## Application notes

- We build our own bootloader (SDK provides source) — full flash is ours.
- MAC address sourced from **efuse** (hardware OTP), not flash. `[bootlog]`
- No dedicated RF calibration partition — country/region set at runtime.
- Full 2 MB available for bootloader + app + OTA + config storage.
- The SDK provides HAL drivers for all peripherals; we build against them.
