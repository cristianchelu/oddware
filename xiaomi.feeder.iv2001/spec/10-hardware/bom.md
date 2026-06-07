# Bill of materials

Active ICs on the single controller PCB. Passive components (resistors,
capacitors, inductors) are not individually catalogued. Board layout refdes,
annotated photos, harness connectors, switches, and test points:
[pcb.md](pcb.md).

| Ref | Part                     | Package  | Manufacturer      | Role                             | Provenance                | Notes                               |
| --- | ------------------------ | -------- | ----------------- | -------------------------------- | ------------------------- | ----------------------------------- |
| U1  | MHCW05P-B (MT7682)       | module   | Xiaomi / MediaTek | Wi-Fi SoC, application processor | `[probe]` `[bootlog]`     | Silicon ID confirmed via boot UART  |
| U2  | AW9523B                  | QFN-24   | Awinic            | I2C 16-ch GPIO expander          | `[probe]` `[ds:AW9523B]`  | Address 0x58 confirmed on bus       |
| U3  | CS1270                   | QFN      | Chipsea           | Load-cell weighing ASSP          | `[probe]` `[bootlog]`     | UART interface, `1270:` in boot log |
| U4  | NC7SB3157P6X             | SOT-363  | onsemi            | SPDT analog switch               | `[probe]`                 | Routes battery or motor to SoC ADC  |
| U5  | SGM42507 (marking SG55B) | SOT-23-5 | SG Micro          | H-bridge motor driver (PH+EN)    | `[probe]` `[ds:SGM42507]` | Drives single auger motor           |


## Gaps

- Power regulator(s) between barrel/battery input and 3.3 V rail not identified.
