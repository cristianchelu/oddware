# AW9523B — I2C GPIO expander

## Summary

16-channel I/O expander (two 8-bit ports) with interrupt output. Connected
to the MT7682 via I2C1 at address **0x58**. Drives motor control, sensors,
buttons, and power enables on this board.

## Interface to host

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus | I2C, 400 kHz max | `[ds:AW9523B §7]` |
| Address | 0x58 (AD0/AD1 tied low) | `[probe]` |
| INT pin | Active-low, open-drain → MT7682 GPIO4 | `[probe]` |
| Reset | Active-low → MT7682 GPIO14 | `[probe]` |
| VCC | 2.5–5.5 V | `[ds:AW9523B §5]` |

## Key registers

| Register | Addr | Function |
|----------|------|----------|
| Input P0 | 0x00 | Read port 0 pin levels |
| Input P1 | 0x01 | Read port 1 pin levels |
| Output P0 | 0x02 | Set port 0 output levels |
| Output P1 | 0x03 | Set port 1 output levels |
| Direction P0 | 0x04 | 0=output, 1=input (per bit) |
| Direction P1 | 0x05 | 0=output, 1=input (per bit) |
| INT enable P0 | 0x06 | 0=enabled, 1=masked |
| Int enable P1 | 0x07 | 0=enabled, 1=masked |
| ID register | 0x10 | Should read 0x23 |
| CTL register | 0x11 | Global config (LED mode, push-pull) |
| LED mode P0 | 0x12 | 0=GPIO, 1=LED current sink |
| LED mode P1 | 0x13 | 0=GPIO, 1=LED current sink |

## Application notes

- All pins used as GPIO on this board (LED mode not used for segment driving;
  the TM1637 handles display independently).
- INT fires on any enabled input change; single interrupt line requires
  reading input registers to determine which pin changed.
- Hardware reset via GPIO14 before init sequence ensures known state.
- Direction register: 0 = output, 1 = input (opposite of some other expanders).
- IRQ-enabled inputs: P0.3 (power button), P0.7 (motor-index IR),
  P1.0 (dispense button), P1.1 (mains present), P1.4 (hopper IR sense).
