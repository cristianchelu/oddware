# Controller PCB

The retail board has **no silkscreen** — no printed U/J/TP labels on the copper.
`pcb_top.png` and `pcb_bottom.png` are annotated with **author-assigned**
designators (`[design]`) so this document, [bom.md](bom.md), and [pinmap.md](pinmap.md)
can be correlated to the physical board.


| View   | File                             |
| ------ | -------------------------------- |
| Top    | [pcb_top.png](pcb_top.png)       |
| Bottom | [pcb_bottom.png](pcb_bottom.png) |


Active IC part numbers and datasheets: [bom.md](bom.md). Electrical pin
assignments: [pinmap.md](pinmap.md).

## Harness connectors (J)


| Ref | Type | Function                   | Provenance | Notes                           |
| --- | ---- | -------------------------- | ---------- | ------------------------------- |
| J1  | 1×6  | Debug / programming UART   | `[probe]`  | **Unpopulated**                 |
| J2  | 1×3  | Motor index wheel IR       | `[probe]`  | AW9523B P0.6 / P0.7             |
| J3  | 1×4  | Hopper full IR             | `[probe]`  | GPIO0 drive, AW9523B P1.4 sense |
| J4  | 1×2  | Auger motor                | `[probe]`  | SGM42507 H-bridge output        |
| J5  | 1×3  | Rear power / reset buttons | `[probe]`  | AW9523B P0.3 / P0.4             |
| J6  | 1×4  | Load cell                  | `[probe]`  | CS1270 bridge                   |
| J7  | 1×2  | Battery                    | `[probe]`  | 4× AA backup pack               |
| J8  | 1×2  | 6 V input                  | `[probe]`  | Barrel adapter                  |


## Switches (SW)


| Ref | Type           | Function                 | Provenance | Notes           |
| --- | -------------- | ------------------------ | ---------- | --------------- |
| SW1 | Tactile switch | BOOT module strap        | `[probe]`  | **Unpopulated**; test pad **TP14** |
| SW2 | Tactile switch | Manual feed press button | `[probe]`  | AW9523B P1.0    |
| SW3 | Tactile switch | RESET module strap       | `[probe]`  | **Unpopulated**; test pad **TP15** (CHIP_EN net) |


## Test points (TP)

All **40** bottom-side pads are labeled **TP1–TP40** on
[pcb_bottom.png](pcb_bottom.png). Characterization is incremental — empty
rows are placeholders for bench follow-up.


| Ref  | Signal / net                          | Provenance | Notes                                                                |
| ---- | ------------------------------------- | ---------- | -------------------------------------------------------------------- |
| TP1  | U1 UART0 TX (MT7682 GPIO21)           | `[probe]`  | Boot ROM / flash tool; use with TP2, TP27; see [flash.md](flash.md) |
| TP2  | U1 UART0 RX (MT7682 GPIO22)           | `[probe]`  | Boot ROM / flash tool; use with TP1, TP27; see [flash.md](flash.md) |
| TP3  | —                                     | —          |                                                                      |
| TP4  | U1 pin 31, GPIO3                      | `[probe]`  | Unused                                                               |
| TP5  | U1 pin 30, GPIO2                      | `[probe]`  | Unused                                                               |
| TP6  | —                                     | —          |                                                                      |
| TP7  | J2 motor-index IR                     | `[probe]`  | Adjacent **J2**; emitter vs receiver TBD                             |
| TP8  | J2 motor-index IR                     | `[probe]`  | Adjacent **J2**; emitter vs receiver TBD                             |
| TP9  | —                                     | —          |                                                                      |
| TP10 | —                                     | —          |                                                                      |
| TP11 | —                                     | —          |                                                                      |
| TP12 | —                                     | —          |                                                                      |
| TP13 | —                                     | —          |                                                                      |
| TP14 | SW1 BOOT module strap                 | `[probe]`  | Unpopulated **SW1**; BOOT strap net — not required for UART0 BROM flash |
| TP15 | SW3 RESET module strap (CHIP_EN)      | `[probe]`  | Unpopulated **SW3**; manual reset during flash — see [flash.md](flash.md) |
| TP16 | —                                     | —          |                                                                      |
| TP17 | —                                     | —          |                                                                      |
| TP18 | J3 hopper full IR (black LED −)       | `[probe]`  | Adjacent **J3**; emitter vs photodiode TBD                           |
| TP19 | J3 hopper full IR (both red +)        | `[probe]`  | **J3** middle two pins shorted here; both emitter/receiver red wires |
| TP20 | J3 hopper full IR (transparent LED −) | `[probe]`  | Adjacent **J3**; emitter vs photodiode TBD                           |
| TP21 | —                                     | —          |                                                                      |
| TP22 | —                                     | —          |                                                                      |
| TP23 | —                                     | —          |                                                                      |
| TP24 | J4 auger motor (black)                | `[probe]`  | Adjacent **J4**; SGM42507 H-bridge output; polarity vs TP25 TBD      |
| TP25 | J4 auger motor (red)                  | `[probe]`  | Adjacent **J4**; SGM42507 H-bridge output; polarity vs TP24 TBD      |
| TP26 | —                                     | —          |                                                                      |
| TP27 | GND                                   | `[probe]`  | Flash / debug ground reference                                       |
| TP28 | —                                     | —          |                                                                      |
| TP29 | —                                     | —          |                                                                      |
| TP30 | —                                     | —          |                                                                      |
| TP31 | —                                     | —          |                                                                      |
| TP32 | J8 DC +6 V                            | `[probe]`  | Adjacent **J8** (barrel adapter)                                     |
| TP33 | J8 DC GND                             | `[probe]`  | Adjacent **J8** (barrel adapter)                                     |
| TP34 | J7 BAT +6 V                           | `[probe]`  | Adjacent **J7** (4× AA backup pack)                                  |
| TP35 | J6 load cell bridge                   | `[probe]`  | Adjacent **J6**; A+/A−/E+/E− assignment TBD                          |
| TP36 | J6 load cell bridge                   | `[probe]`  | Adjacent **J6**; A+/A−/E+/E− assignment TBD                          |
| TP37 | J6 load cell bridge                   | `[probe]`  | Adjacent **J6**; A+/A−/E+/E− assignment TBD                          |
| TP38 | J6 load cell bridge                   | `[probe]`  | Adjacent **J6**; A+/A−/E+/E− assignment TBD                          |
| TP39 | J5 rear **Power** button              | `[probe]`  | Adjacent **J5**; AW9523B P0.3 `[probe]`                              |
| TP40 | J5 rear recessed **Reset** button     | `[probe]`  | Adjacent **J5**; AW9523B P0.4 `[probe]`                              |


## Front display module

The TM1637 7-segment driver is **not on the controller PCB** — it is almost
certainly inside the front LCD module plastic and was not desoldered for
inspection. `[probe]` See [components/display-tm1637.md](components/display-tm1637.md)
for protocol and pin usage (MT7682 GPIO1/13, AW9523B P0.5 power rail).