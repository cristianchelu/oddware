# Pin map

U designators match [bom.md](bom.md). Annotated board photos and J/SW/TP
refdes: [pcb.md](pcb.md).

## MT7682 (U1) — GPIO 0–22

Pins used by the application firmware. Directly exposed on the module.


| GPIO  | Direction / Mode | Function                               | Provenance            |
| ----- | ---------------- | -------------------------------------- | --------------------- |
| 0     | Alt / Input      | Hopper low-fill IR drive (PWM)         | `[probe]` `[bootlog]` |
| 1     | Output           | TM1637 DIO (bit-bang)                  | `[probe]`             |
| 2     | UART1 TX         | Factory/debug UART TX                  | `[bootlog]`           |
| 3     | UART1 RX         | Factory/debug UART RX                  | `[bootlog]`           |
| 4     | EINT Input       | AW9523B INT (expander interrupt)       | `[probe]`             |
| 5–10  | —                | No application use identified          | —                     |
| 11    | UART2 TX         | CS1270 weigh ASSP TX                   | `[probe]` `[bootlog]` |
| 12    | UART2 RX         | CS1270 weigh ASSP RX                   | `[probe]` `[bootlog]` |
| 13    | Output           | TM1637 CLK (bit-bang)                  | `[probe]`             |
| 14    | Output           | AW9523B hardware RESET (active-low)    | `[probe]`             |
| 15    | I2C1 SCL         | AW9523B clock (`HAL_GPIO_15_SCL1`)     | `[probe]`             |
| 16    | I2C1 SDA         | AW9523B data (`HAL_GPIO_16_SDA1`)      | `[probe]`             |
| 17    | ADC              | NC7SB3157 COM — battery or motor sense | `[probe]`             |
| 18–20 | —                | No application use identified          | —                     |
| 21    | UART0 TX         | Boot ROM / flash tool (module pad)     | `[bootlog]`           |
| 22    | UART0 RX         | Boot ROM / flash tool (module pad)     | `[bootlog]`           |


## AW9523B (U2) — I2C GPIO expander @ 0x58

16 pins: Port P0 (bits 0–7) and Port P1 (bits 8–15).

### Port P0


| Pin  | Mask   | Dir    | Function                           | Provenance       |
| ---- | ------ | ------ | ---------------------------------- | ---------------- |
| P0.0 | 0x0001 | OUT    | SGM42507 PH (motor direction)      | `[probe]`        |
| P0.1 | 0x0002 | OUT    | SGM42507 EN (motor enable)         | `[probe]`        |
| P0.2 | 0x0004 | OUT    | CS1270 power enable                | `[probe-needed]` |
| P0.3 | 0x0008 | IN+IRQ | Rear power button (wake)           | `[probe]`        |
| P0.4 | 0x0010 | IN     | Pin-hole reset button (active low) | `[probe]`        |
| P0.5 | 0x0020 | OUT    | TM1637 / front panel power rail    | `[probe]`        |
| P0.6 | 0x0040 | OUT    | Motor-index IR LED (index disk)    | `[probe]`        |
| P0.7 | 0x0080 | IN+IRQ | Motor-index IR detector            | `[probe]`        |


### Port P1


| Pin  | Mask   | Dir    | Function                                   | Provenance |
| ---- | ------ | ------ | ------------------------------------------ | ---------- |
| P1.0 | 0x0100 | IN+IRQ | Manual dispense button (front)             | `[probe]`  |
| P1.1 | 0x0200 | IN+IRQ | DC / mains present sense                   | `[probe]`  |
| P1.2 | 0x0400 | IN     | Factory test UART (unused in product)      | `[probe]`  |
| P1.3 | 0x0800 | IN     | Factory test UART (unused in product)      | `[probe]`  |
| P1.4 | 0x1000 | IN+IRQ | Hopper low-fill IR sense                   | `[probe]`  |
| P1.5 | —      | —      | No assignment identified                   | —          |
| P1.6 | —      | —      | No assignment identified                   | —          |
| P1.7 | 0x8000 | OUT    | NC7SB3157 select (high=battery, low=motor) | `[probe]`  |


## Signal routing summary

```
MT7682 GPIO0 ──── hopper low-fill IR drive
MT7682 GPIO1,13 ─ TM1637 display (DIO, CLK)
MT7682 GPIO4 ──── AW9523B INT
MT7682 GPIO11,12  CS1270 UART2
MT7682 GPIO14–16  AW9523B (RST, SCL, SDA)
MT7682 GPIO17 ─── NC7SB3157 COM (ADC input)
MT7682 GPIO21,22  UART0 boot/flash

AW9523B P0.0,P0.1 ── SGM42507 (PH, EN)
AW9523B P0.2 ──────── CS1270 power
AW9523B P0.3 ──────── rear power button
AW9523B P0.4 ──────── pin-hole reset
AW9523B P0.5 ──────── display power rail
AW9523B P0.6,P0.7 ── motor-index IR (LED, detector)
AW9523B P1.0 ──────── manual dispense button
AW9523B P1.1 ──────── mains present
AW9523B P1.4 ──────── hopper low-fill IR sense
AW9523B P1.7 ──────── NC7SB3157 S (mux select)
```

